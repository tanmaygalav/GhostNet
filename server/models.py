import uuid
from datetime import datetime, timezone
from sqlalchemy import Column, String, Text, DateTime, Integer, ForeignKey
from sqlalchemy.orm import relationship
from database import Base


def utc_now() -> datetime:
    return datetime.now(timezone.utc)


def generate_uuid() -> str:
    return str(uuid.uuid4())


class Project(Base):
    __tablename__ = "projects"

    id = Column(String(36), primary_key=True, default=generate_uuid)
    name = Column(String(128), nullable=False)
    description = Column(Text, nullable=True)
    created_at = Column(DateTime(timezone=True), default=utc_now, nullable=False)
    updated_at = Column(
        DateTime(timezone=True),
        default=utc_now,
        onupdate=utc_now,
        nullable=False,
    )

    scenarios = relationship(
        "Scenario", back_populates="project", cascade="all, delete-orphan"
    )


class Scenario(Base):
    __tablename__ = "scenarios"

    id = Column(String(36), primary_key=True, default=generate_uuid)
    project_id = Column(
        String(36), ForeignKey("projects.id", ondelete="CASCADE"), nullable=False
    )
    name = Column(String(128), nullable=False)
    description = Column(Text, nullable=True)
    created_at = Column(DateTime(timezone=True), default=utc_now, nullable=False)
    updated_at = Column(
        DateTime(timezone=True),
        default=utc_now,
        onupdate=utc_now,
        nullable=False,
    )

    project = relationship("Project", back_populates="scenarios")
    versions = relationship(
        "ScenarioVersion", back_populates="scenario", cascade="all, delete-orphan"
    )


class ScenarioVersion(Base):
    __tablename__ = "scenario_versions"

    id = Column(String(36), primary_key=True, default=generate_uuid)
    scenario_id = Column(
        String(36), ForeignKey("scenarios.id", ondelete="CASCADE"), nullable=False
    )
    version_number = Column(Integer, nullable=False)
    configuration_json = Column(Text, nullable=False)
    created_at = Column(DateTime(timezone=True), default=utc_now, nullable=False)

    scenario = relationship("Scenario", back_populates="versions")
    runs = relationship(
        "SimulationRun", back_populates="scenario_version", cascade="all, delete-orphan"
    )


class SimulationRun(Base):
    __tablename__ = "simulation_runs"

    id = Column(String(36), primary_key=True, default=generate_uuid)
    scenario_version_id = Column(
        String(36),
        ForeignKey("scenario_versions.id", ondelete="CASCADE"),
        nullable=False,
    )
    status = Column(String(32), default="QUEUED", nullable=False)
    random_seed = Column(Integer, nullable=False)
    engine_version = Column(String(32), default="0.1.0", nullable=False)
    simulated_duration_us = Column(Integer, default=0, nullable=False)
    processed_events = Column(Integer, default=0, nullable=False)
    metrics_summary_json = Column(Text, nullable=True)
    created_at = Column(DateTime(timezone=True), default=utc_now, nullable=False)
    completed_at = Column(DateTime(timezone=True), nullable=True)

    scenario_version = relationship("ScenarioVersion", back_populates="runs")