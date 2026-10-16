"use client";

import React from "react";

interface MetricsProps {
  metrics: Record<string, any> | null;
  virtualDurationUs: number;
  processedEvents: number;
}

export const BottomPanel: React.FC<MetricsProps> = ({
  metrics,
  virtualDurationUs,
  processedEvents,
}) => {
  if (!metrics) {
    return (
      <footer className="gn-bottombar" style={{ justifyContent: "center", alignItems: "center" }}>
        <div style={{ fontSize: "12px", color: "#64748b" }}>
          Ready to simulate. Click &quot;Run Simulation&quot; above to execute topology.
        </div>
      </footer>
    );
  }

  const statCards = [
    { label: "Generated", value: metrics.total_generated ?? 0, color: "#f8fafc" },
    { label: "Completed", value: metrics.total_completed ?? 0, color: "#10b981" },
    { label: "Failed", value: metrics.total_failed ?? 0, color: "#f43f5e" },
    {
      label: "Success Rate",
      value: `${((metrics.completion_rate ?? 0) * 100).toFixed(1)}%`,
      color: "#06b6d4",
    },
    {
      label: "Mean Latency",
      value: `${((metrics.mean_latency_us ?? 0) / 1000).toFixed(2)} ms`,
      color: "#f59e0b",
    },
    {
      label: "p95 Latency",
      value: `${((metrics.p95_latency_us ?? 0) / 1000).toFixed(2)} ms`,
      color: "#f59e0b",
    },
    { label: "Loss Drops", value: metrics.dropped_by_loss ?? 0, color: "#f43f5e" },
    { label: "Queue Overflows", value: metrics.queue_overflows ?? 0, color: "#f43f5e" },
    { label: "Service Outage", value: metrics.service_down_failures ?? 0, color: "#f43f5e" },
  ];

  return (
    <footer className="gn-bottombar">
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", borderBottom: "1px solid #1e293b", paddingBottom: "6px" }}>
        <span style={{ fontSize: "11px", fontWeight: 700, letterSpacing: "1px", textTransform: "uppercase", color: "#06b6d4" }}>
          Simulation Telemetry
        </span>
        <div style={{ display: "flex", gap: "16px", fontFamily: "monospace", fontSize: "11px", color: "#64748b" }}>
          <span>Virtual Duration: {(virtualDurationUs / 1000).toFixed(1)} ms</span>
          <span>Processed Events: {processedEvents}</span>
        </div>
      </div>

      <div style={{ display: "flex", gap: "8px", overflowX: "auto", marginTop: "6px" }}>
        {statCards.map((card, idx) => (
          <div key={idx} className="gn-stat-card" style={{ flex: "1 1 0", minWidth: "90px" }}>
            <div style={{ fontSize: "10px", color: "#64748b", whiteSpace: "nowrap" }}>{card.label}</div>
            <div style={{ fontSize: "13px", fontWeight: 700, fontFamily: "monospace", color: card.color, marginTop: "2px" }}>
              {card.value}
            </div>
          </div>
        ))}
      </div>
    </footer>
  );
};