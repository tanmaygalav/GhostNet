from fastapi import FastAPI, Depends, HTTPException, status
from fastapi.middleware.cors import CORSMiddleware
from sqlalchemy.orm import Session
from typing import List
import json
from datetime import datetime, timezone

from database import engine, Base, get_db
import models
import schemas
from engine_runner import run_simulation, EngineExecutionError

Base.metadata.create_all(bind=engine)

app = FastAPI(
    title="GHOSTNET API",
    version="0.1.0",
    description="Programmable Network Simulation and Resilience Engineering Platform",
)

# Enable CORS for frontend dev servers
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:3000", "http://127.0.0.1:3000"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# --- Projects Endpoints ---
@app.post(
    "/api/v1/projects",
    response_model=schemas.ProjectResponse,
    status_code=status.HTTP_201_CREATED,
)
def create_project(project_in: schemas.ProjectCreate, db: Session = Depends(get_db)):
    project = models.Project(name=project_in.name, description=project_in.description)
    db.add(project)
    db.commit()
    db.refresh(project)
    return project


@app.get("/api/v1/projects", response_model=List[schemas.ProjectResponse])
def list_projects(db: Session = Depends(get_db)):
    return db.query(models.Project).all()


@app.get("/api/v1/projects/{project_id}", response_model=schemas.ProjectResponse)
def get_project(project_id: str, db: Session = Depends(get_db)):
    project = db.query(models.Project).filter(models.Project.id == project_id).first()
    if not project:
        raise HTTPException(status_code=404, detail="Project not found")
    return project


# --- Scenarios Endpoints ---
@app.post(
    "/api/v1/projects/{project_id}/scenarios",
    response_model=schemas.ScenarioResponse,
    status_code=status.HTTP_201_CREATED,
)
def create_scenario(
    project_id: str,
    scenario_in: schemas.ScenarioCreate,
    db: Session = Depends(get_db),
):
    project = db.query(models.Project).filter(models.Project.id == project_id).first()
    if not project:
        raise HTTPException(status_code=404, detail="Project not found")

    scenario = models.Scenario(
        project_id=project_id,
        name=scenario_in.name,
        description=scenario_in.description,
    )
    db.add(scenario)
    db.flush()

    version = models.ScenarioVersion(
        scenario_id=scenario.id,
        version_number=1,
        configuration_json=json.dumps(scenario_in.configuration),
    )
    db.add(version)
    db.commit()
    db.refresh(scenario)

    return schemas.ScenarioResponse(
        id=scenario.id,
        project_id=scenario.project_id,
        name=scenario.name,
        description=scenario.description,
        latest_version=1,
        created_at=scenario.created_at,
        updated_at=scenario.updated_at,
    )


@app.put("/api/v1/scenarios/{scenario_id}/versions", response_model=schemas.ScenarioResponse)
def create_scenario_version(
    scenario_id: str,
    update_in: schemas.ScenarioUpdate,
    db: Session = Depends(get_db),
):
    scenario = db.query(models.Scenario).filter(models.Scenario.id == scenario_id).first()
    if not scenario:
        raise HTTPException(status_code=404, detail="Scenario not found")

    latest_ver = (
        db.query(models.ScenarioVersion)
        .filter(models.ScenarioVersion.scenario_id == scenario_id)
        .order_by(models.ScenarioVersion.version_number.desc())
        .first()
    )
    next_ver_num = (latest_ver.version_number + 1) if latest_ver else 1

    new_version = models.ScenarioVersion(
        scenario_id=scenario.id,
        version_number=next_ver_num,
        configuration_json=json.dumps(update_in.configuration),
    )
    db.add(new_version)
    scenario.updated_at = datetime.now(timezone.utc)
    db.commit()
    db.refresh(scenario)

    return schemas.ScenarioResponse(
        id=scenario.id,
        project_id=scenario.project_id,
        name=scenario.name,
        description=scenario.description,
        latest_version=next_ver_num,
        created_at=scenario.created_at,
        updated_at=scenario.updated_at,
    )


@app.get("/api/v1/scenarios/{scenario_id}/export")
def export_scenario(scenario_id: str, db: Session = Depends(get_db)):
    scenario = db.query(models.Scenario).filter(models.Scenario.id == scenario_id).first()
    if not scenario:
        raise HTTPException(status_code=404, detail="Scenario not found")

    latest_ver = (
        db.query(models.ScenarioVersion)
        .filter(models.ScenarioVersion.scenario_id == scenario_id)
        .order_by(models.ScenarioVersion.version_number.desc())
        .first()
    )
    if not latest_ver:
        raise HTTPException(status_code=404, detail="Scenario has no versions")

    return {
        "scenario_name": scenario.name,
        "version": latest_ver.version_number,
        "configuration": json.loads(latest_ver.configuration_json),
    }


# --- Simulation Runs Endpoints ---
@app.post(
    "/api/v1/scenarios/{scenario_id}/runs",
    response_model=schemas.RunResponse,
    status_code=status.HTTP_201_CREATED,
)
def execute_run(
    scenario_id: str,
    run_in: schemas.RunCreate,
    db: Session = Depends(get_db),
):
    scenario = db.query(models.Scenario).filter(models.Scenario.id == scenario_id).first()
    if not scenario:
        raise HTTPException(status_code=404, detail="Scenario not found")

    query = db.query(models.ScenarioVersion).filter(models.ScenarioVersion.scenario_id == scenario_id)
    if run_in.version_number is not None:
        target_version = query.filter(models.ScenarioVersion.version_number == run_in.version_number).first()
    else:
        target_version = query.order_by(models.ScenarioVersion.version_number.desc()).first()

    if not target_version:
        raise HTTPException(status_code=400, detail="Target scenario version not found")

    scenario_dict = json.loads(target_version.configuration_json)
    seed = run_in.random_seed or 42

    try:
        run_status, engine_output = run_simulation(scenario_dict, override_seed=seed)
    except EngineExecutionError as err:
        raise HTTPException(status_code=500, detail=f"Simulation execution error: {str(err)}")

    run = models.SimulationRun(
        scenario_version_id=target_version.id,
        status=run_status,
        random_seed=seed,
        engine_version="0.1.0",
        simulated_duration_us=engine_output.get("virtual_duration_us", 0),
        processed_events=engine_output.get("processed_events", 0),
        metrics_summary_json=json.dumps(engine_output.get("metrics", {})),
        completed_at=datetime.now(timezone.utc),
    )
    db.add(run)
    db.commit()
    db.refresh(run)

    return schemas.RunResponse(
        id=run.id,
        scenario_version_id=run.scenario_version_id,
        status=run.status,
        random_seed=run.random_seed,
        engine_version=run.engine_version,
        simulated_duration_us=run.simulated_duration_us,
        processed_events=run.processed_events,
        metrics=json.loads(run.metrics_summary_json) if run.metrics_summary_json else {},
        created_at=run.created_at,
        completed_at=run.completed_at,
    )


@app.get("/api/v1/runs/{run_id}", response_model=schemas.RunResponse)
def get_run(run_id: str, db: Session = Depends(get_db)):
    run = db.query(models.SimulationRun).filter(models.SimulationRun.id == run_id).first()
    if not run:
        raise HTTPException(status_code=404, detail="Run not found")

    return schemas.RunResponse(
        id=run.id,
        scenario_version_id=run.scenario_version_id,
        status=run.status,
        random_seed=run.random_seed,
        engine_version=run.engine_version,
        simulated_duration_us=run.simulated_duration_us,
        processed_events=run.processed_events,
        metrics=json.loads(run.metrics_summary_json) if run.metrics_summary_json else {},
        created_at=run.created_at,
        completed_at=run.completed_at,
    )


# --- Experiments Comparison ---
@app.post("/api/v1/experiments/compare", response_model=schemas.ComparisonResponse)
def compare_runs(req: schemas.CompareRequest, db: Session = Depends(get_db)):
    base_run = db.query(models.SimulationRun).filter(models.SimulationRun.id == req.baseline_run_id).first()
    treat_run = db.query(models.SimulationRun).filter(models.SimulationRun.id == req.treatment_run_id).first()

    if not base_run or not treat_run:
        raise HTTPException(status_code=404, detail="One or both simulation runs not found")

    base_m = json.loads(base_run.metrics_summary_json or "{}")
    treat_m = json.loads(treat_run.metrics_summary_json or "{}")

    comparable_keys = [
        "total_generated",
        "total_completed",
        "total_failed",
        "completion_rate",
        "mean_latency_us",
        "p50_latency_us",
        "p95_latency_us",
        "p99_latency_us",
        "service_down_failures",
        "queue_overflows",
        "dropped_by_loss",
    ]

    deltas: dict = {}
    for key in comparable_keys:
        b_val = float(base_m.get(key, 0.0))
        t_val = float(treat_m.get(key, 0.0))
        abs_diff = t_val - b_val
        rel_pct = ((abs_diff / b_val) * 100.0) if b_val != 0.0 else None

        deltas[key] = schemas.MetricDelta(
            baseline=b_val,
            treatment=t_val,
            absolute_change=abs_diff,
            relative_change_pct=rel_pct,
        )

    return schemas.ComparisonResponse(
        baseline_run_id=base_run.id,
        treatment_run_id=treat_run.id,
        deltas=deltas,
    )