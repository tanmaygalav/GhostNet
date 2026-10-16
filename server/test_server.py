import pytest
from fastapi.testclient import TestClient
from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker
from sqlalchemy.pool import StaticPool

from database import Base, get_db
from main import app
import models

# Use StaticPool so all threads share the exact same in-memory SQLite instance
SQLALCHEMY_DATABASE_URL = "sqlite:///:memory:"
test_engine = create_engine(
    SQLALCHEMY_DATABASE_URL,
    connect_args={"check_same_thread": False},
    poolclass=StaticPool,
)
TestingSessionLocal = sessionmaker(
    autocommit=False, autoflush=False, bind=test_engine
)

# Create all tables on the shared in-memory database
Base.metadata.create_all(bind=test_engine)


def override_get_db():
    db = TestingSessionLocal()
    try:
        yield db
    finally:
        db.close()


app.dependency_overrides[get_db] = override_get_db
client = TestClient(app)


def test_full_project_scenario_and_run_pipeline():
    # 1. Create a Project
    proj_resp = client.post(
        "/api/v1/projects",
        json={"name": "Alpha Project", "description": "Testing FastAPI layer"},
    )
    assert proj_resp.status_code == 201
    proj_data = proj_resp.json()
    project_id = proj_data["id"]
    assert proj_data["name"] == "Alpha Project"

    # 2. Create a Scenario with valid topology and fault configuration
    scenario_payload = {
        "name": "Outage Test Scenario",
        "description": "Baseline vs Outage",
        "configuration": {
            "config": {
                "seed": 42,
                "max_duration_us": 2000000,
                "max_events": 50000,
            },
            "nodes": [
                {"id": 1, "name": "Client", "type": "CLIENT"},
                {
                    "id": 2,
                    "name": "Backend",
                    "type": "SERVICE",
                    "processing_delay_us": 4000,
                    "max_concurrency": 2,
                    "queue_capacity": 5,
                },
            ],
            "links": [
                {
                    "id": 10,
                    "source_id": 1,
                    "target_id": 2,
                    "propagation_delay_us": 6000,
                }
            ],
            "workloads": [
                {
                    "source_id": 1,
                    "target_id": 2,
                    "arrival_rate_rps": 10.0,
                    "duration_us": 2000000,
                }
            ],
            "faults": [
                {
                    "id": 1,
                    "type": "SERVICE_OUTAGE",
                    "target_component_id": 2,
                    "start_time_us": 500000,
                    "duration_us": 500000,
                }
            ],
        },
    }

    scen_resp = client.post(
        f"/api/v1/projects/{project_id}/scenarios", json=scenario_payload
    )
    assert scen_resp.status_code == 201
    scen_data = scen_resp.json()
    scenario_id = scen_data["id"]
    assert scen_data["latest_version"] == 1

    # 3. Trigger simulation execution via C++ engine subprocess
    run_resp = client.post(
        f"/api/v1/scenarios/{scenario_id}/runs", json={"random_seed": 777}
    )
    assert run_resp.status_code == 201
    run_data = run_resp.json()

    assert run_data["status"] == "COMPLETED"
    assert run_data["random_seed"] == 777
    assert run_data["processed_events"] > 0
    assert "metrics" in run_data
    assert run_data["metrics"]["total_generated"] > 0
    assert run_data["metrics"]["service_down_failures"] > 0

    # 4. Fetch persisted run details
    run_id = run_data["id"]
    fetch_resp = client.get(f"/api/v1/runs/{run_id}")
    assert fetch_resp.status_code == 200
    assert fetch_resp.json()["id"] == run_id
    

def test_scenario_versioning_and_immutability():
    # 1. Create Project
    p = client.post("/api/v1/projects", json={"name": "Versioning Proj"}).json()
    p_id = p["id"]

    base_config = {
        "config": {"seed": 1, "max_duration_us": 1000000},
        "nodes": [
            {"id": 1, "name": "C", "type": "CLIENT"},
            {"id": 2, "name": "S", "type": "SERVICE", "processing_delay_us": 1000},
        ],
        "links": [{"id": 1, "source_id": 1, "target_id": 2, "propagation_delay_us": 2000}],
        "workloads": [{"source_id": 1, "target_id": 2, "arrival_rate_rps": 5.0, "duration_us": 1000000}],
    }

    # 2. Create Scenario v1
    scen = client.post(
        f"/api/v1/projects/{p_id}/scenarios",
        json={"name": "Versioning Scen", "configuration": base_config},
    ).json()
    s_id = scen["id"]
    assert scen["latest_version"] == 1

    # 3. Create Scenario v2 (updating delay)
    updated_config = dict(base_config)
    updated_config["nodes"][1]["processing_delay_us"] = 8000
    v2_resp = client.put(f"/api/v1/scenarios/{s_id}/versions", json={"configuration": updated_config}).json()
    assert v2_resp["latest_version"] == 2

    # 4. Run v1 explicitly
    run_v1 = client.post(f"/api/v1/scenarios/{s_id}/runs", json={"random_seed": 10, "version_number": 1}).json()
    # 5. Run v2 explicitly
    run_v2 = client.post(f"/api/v1/scenarios/{s_id}/runs", json={"random_seed": 10, "version_number": 2}).json()

    # Mean latency for v1: 2000 + 1000 = 3000
    # Mean latency for v2: 2000 + 8000 = 10000
    assert run_v1["metrics"]["mean_latency_us"] == 3000.0
    assert run_v2["metrics"]["mean_latency_us"] == 10000.0

    # 6. Test Comparison API (FR-044)
    cmp_resp = client.post(
        "/api/v1/experiments/compare",
        json={"baseline_run_id": run_v1["id"], "treatment_run_id": run_v2["id"]},
    )
    assert cmp_resp.status_code == 200
    cmp_data = cmp_resp.json()
    latency_delta = cmp_data["deltas"]["mean_latency_us"]
    assert latency_delta["baseline"] == 3000.0
    assert latency_delta["treatment"] == 10000.0
    assert latency_delta["absolute_change"] == 7000.0