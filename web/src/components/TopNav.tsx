"use client";

import React from "react";
import { Play, RotateCcw, Activity } from "lucide-react";

interface TopNavProps {
  onRun: () => void;
  onReset: () => void;
  isRunning: boolean;
  status: string;
}

export const TopNav: React.FC<TopNavProps> = ({
  onRun,
  onReset,
  isRunning,
  status,
}) => {
  return (
    <header className="gn-topbar">
      <div style={{ display: "flex", alignItems: "center", gap: "12px" }}>
        <div
          style={{
            width: "32px",
            height: "32px",
            borderRadius: "6px",
            background: "rgba(6, 182, 212, 0.12)",
            border: "1px solid rgba(6, 182, 212, 0.3)",
            color: "#06b6d4",
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
          }}
        >
          <Activity style={{ width: "18px", height: "18px" }} />
        </div>
        <div>
          <div style={{ display: "flex", alignItems: "center", gap: "8px" }}>
            <span style={{ fontSize: "14px", fontWeight: 700, letterSpacing: "0.5px", color: "#f8fafc" }}>
              GHOSTNET
            </span>
            <span
              style={{
                fontSize: "10px",
                fontFamily: "monospace",
                padding: "1px 6px",
                borderRadius: "4px",
                background: "#151d30",
                border: "1px solid #243048",
                color: "#06b6d4",
              }}
            >
              v0.1.0
            </span>
          </div>
          <div style={{ fontSize: "11px", color: "#64748b" }}>
            Programmable Resilience Simulator
          </div>
        </div>
      </div>

      <div style={{ display: "flex", alignItems: "center", gap: "14px" }}>
        <div style={{ display: "flex", alignItems: "center", gap: "6px", fontSize: "12px" }}>
          <span style={{ color: "#64748b" }}>Status:</span>
          <span
            style={{
              fontFamily: "monospace",
              fontSize: "11px",
              padding: "2px 8px",
              borderRadius: "4px",
              background:
                status === "COMPLETED"
                  ? "rgba(16, 185, 129, 0.15)"
                  : isRunning
                  ? "rgba(245, 158, 11, 0.15)"
                  : "#151d30",
              color:
                status === "COMPLETED"
                  ? "#10b981"
                  : isRunning
                  ? "#f59e0b"
                  : "#94a3b8",
              border: `1px solid ${
                status === "COMPLETED"
                  ? "rgba(16, 185, 129, 0.35)"
                  : isRunning
                  ? "rgba(245, 158, 11, 0.35)"
                  : "#243048"
              }`,
            }}
          >
            {status}
          </span>
        </div>

        <button
          onClick={onReset}
          style={{
            display: "flex",
            alignItems: "center",
            gap: "6px",
            padding: "6px 12px",
            fontSize: "12px",
            color: "#cbd5e1",
            background: "#151d30",
            border: "1px solid #243048",
            borderRadius: "6px",
            cursor: "pointer",
          }}
        >
          <RotateCcw style={{ width: "13px", height: "13px" }} />
          Reset
        </button>

        <button
          onClick={onRun}
          disabled={isRunning}
          style={{
            display: "flex",
            alignItems: "center",
            gap: "8px",
            padding: "6px 16px",
            fontSize: "12px",
            fontWeight: 600,
            color: "#080c14",
            background: isRunning ? "#0891b2" : "#06b6d4",
            border: "none",
            borderRadius: "6px",
            cursor: isRunning ? "not-allowed" : "pointer",
            boxShadow: "0 0 14px rgba(6, 182, 212, 0.4)",
          }}
        >
          <Play style={{ width: "14px", height: "14px", fill: "#080c14" }} />
          {isRunning ? "Simulating..." : "Run Simulation"}
        </button>
      </div>
    </header>
  );
};