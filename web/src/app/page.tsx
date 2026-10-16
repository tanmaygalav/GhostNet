"use client";

import React, { useState, useMemo } from "react";
import {
  ReactFlow,
  Background,
  Controls,
  BackgroundVariant,
} from "@xyflow/react";
import "@xyflow/react/dist/style.css";

import { useTopologyStore } from "@/store/topologyStore";
import { TopNav } from "@/components/TopNav";
import { LeftSidebar } from "@/components/LeftSidebar";
import { RightInspector } from "@/components/RightInspector";
import { BottomPanel } from "@/components/BottomPanel";
import { CustomNode } from "@/components/CustomNode";

export default function WorkspacePage() {
  const {
    nodes,
    edges,
    onNodesChange,
    onEdgesChange,
    onConnect,
    setSelectedElement,
    exportScenarioPayload,
    resetToTemplate,
  } = useTopologyStore();

  const nodeTypes = useMemo(() => ({ custom: CustomNode }), []);

  const [isRunning, setIsRunning] = useState(false);
  const [status, setStatus] = useState("IDLE");
  const [metrics, setMetrics] = useState<Record<string, any> | null>(null);
  const [virtualDuration, setVirtualDuration] = useState(0);
  const [processedEvents, setProcessedEvents] = useState(0);

  const handleRunSimulation = async () => {
    try {
      setIsRunning(true);
      setStatus("RUNNING");

      const scenarioPayload = exportScenarioPayload();

      const projRes = await fetch("http://localhost:8000/api/v1/projects", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          name: "Canvas Session",
          description: "Execution from Next.js UI",
        }),
      });
      const project = await projRes.json();

      const scenRes = await fetch(
        `http://localhost:8000/api/v1/projects/${project.id}/scenarios`,
        {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({
            name: "Interactive Scenario",
            configuration: scenarioPayload,
          }),
        }
      );
      const scenario = await scenRes.json();

      const runRes = await fetch(
        `http://localhost:8000/api/v1/scenarios/${scenario.id}/runs`,
        {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ random_seed: 42 }),
        }
      );
      const runData = await runRes.json();

      setMetrics(runData.metrics);
      setVirtualDuration(runData.simulated_duration_us);
      setProcessedEvents(runData.processed_events);
      setStatus(runData.status);
    } catch (err) {
      console.error("Simulation request failed:", err);
      setStatus("FAILED");
    } finally {
      setIsRunning(false);
    }
  };

  return (
    <div style={{ display: "flex", flexDirection: "column", height: "100vh", width: "100vw", overflow: "hidden", background: "#080c14" }}>
      <TopNav
        onRun={handleRunSimulation}
        onReset={resetToTemplate}
        isRunning={isRunning}
        status={status}
      />

      <div style={{ display: "flex", flex: 1, overflow: "hidden", position: "relative" }}>
        <LeftSidebar />

        <main style={{ flex: 1, height: "100%", position: "relative" }}>
          <ReactFlow
            nodes={nodes}
            edges={edges}
            nodeTypes={nodeTypes}
            onNodesChange={onNodesChange}
            onEdgesChange={onEdgesChange}
            onConnect={onConnect}
            onNodeClick={(_, node) =>
              setSelectedElement({ id: node.id, type: "node" })
            }
            onEdgeClick={(_, edge) =>
              setSelectedElement({ id: edge.id, type: "edge" })
            }
            onPaneClick={() => setSelectedElement(null)}
            fitView
          >
            <Background
              variant={BackgroundVariant.Dots}
              gap={16}
              size={1}
              color="#1e293b"
            />
            <Controls />
          </ReactFlow>
        </main>

        <RightInspector />
      </div>

      <BottomPanel
        metrics={metrics}
        virtualDurationUs={virtualDuration}
        processedEvents={processedEvents}
      />
    </div>
  );
}