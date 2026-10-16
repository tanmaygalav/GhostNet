from pydantic import BaseModel, Field, ConfigDict
from typing import Optional, Dict, Any, List
from datetime import datetime


# Project Schemas
class ProjectCreate(BaseModel):
    name: str = Field(..., min_length=1, max_length=128)
    description: Optional[str] = None


class ProjectResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: str
    name: str
    description: Optional[str]
    created_at: datetime
    updated_at: datetime


# Scenario Schemas
class ScenarioCreate(BaseModel):
    name: str = Field(..., min_length=1, max_length=128)
    description: Optional[str] = None
    configuration: Dict[str, Any] = Field(..., description="Full scenario JSON configuration")


class ScenarioUpdate(BaseModel):
    configuration: Dict[str, Any] = Field(..., description="Updated scenario configuration that creates a new version")


class ScenarioVersionResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: str
    scenario_id: str
    version_number: int
    configuration_json: str
    created_at: datetime


class ScenarioResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: str
    project_id: str
    name: str
    description: Optional[str]
    latest_version: int
    created_at: datetime
    updated_at: datetime


# Simulation Run Schemas
class RunCreate(BaseModel):
    random_seed: Optional[int] = 42
    version_number: Optional[int] = None  # None selects latest version


class RunResponse(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: str
    scenario_version_id: str
    status: str
    random_seed: int
    engine_version: str
    simulated_duration_us: int
    processed_events: int
    metrics: Optional[Dict[str, Any]] = None
    created_at: datetime
    completed_at: Optional[datetime] = None


# Experiment Comparison Schemas
class CompareRequest(BaseModel):
    baseline_run_id: str
    treatment_run_id: str


class MetricDelta(BaseModel):
    baseline: float
    treatment: float
    absolute_change: float
    relative_change_pct: Optional[float] = None


class ComparisonResponse(BaseModel):
    baseline_run_id: str
    treatment_run_id: str
    deltas: Dict[str, MetricDelta]