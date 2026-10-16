"use client";

import React, { memo } from "react";
import { Handle, Position, NodeProps } from "@xyflow/react";
import { Monitor, Server, Cpu, Split } from "lucide-react";
import { NodeData } from "@/store/topologyStore";

const icons = {
  CLIENT: Monitor,
  SERVICE: Server,
  ROUTER: Cpu,
  LOAD_BALANCER: Split,
};

const badgeColors = {
  CLIENT: "bg-cyan-500/10 text-cyan-400 border-cyan-500/30",
  SERVICE: "bg-emerald-500/10 text-emerald-400 border-emerald-500/30",
  ROUTER: "bg-purple-500/10 text-purple-400 border-purple-500/30",
  LOAD_BALANCER: "bg-amber-500/10 text-amber-400 border-amber-500/30",
};

export const CustomNode = memo(({ data, selected }: NodeProps) => {
  const nodeData = (data ?? {}) as unknown as NodeData;
  const Icon = icons[nodeData.type] || Server;
  const badgeClass = badgeColors[nodeData.type] || badgeColors.SERVICE;

  return (
    <div
      style={{
        background: "#111827",
        border: selected ? "1.5px solid #06b6d4" : "1px solid #1f293d",
        boxShadow: selected ? "0 0 16px rgba(6, 182, 212, 0.35)" : "0 4px 12px rgba(0,0,0,0.4)",
        borderRadius: "8px",
        padding: "10px 14px",
        minWidth: "160px",
        color: "#f8fafc",
        cursor: "pointer",
      }}
    >
      {/* Input target handle on Left */}
      <Handle type="target" position={Position.Left} style={{ left: -4 }} />

      <div style={{ display: "flex", alignItems: "center", gap: "10px" }}>
        <div
          style={{
            padding: "6px",
            borderRadius: "6px",
            background: "#161f30",
            border: "1px solid #243048",
            color: "#06b6d4",
            display: "flex",
          }}
        >
          <Icon style={{ width: "16px", height: "16px" }} />
        </div>

        <div style={{ flex: 1 }}>
          <div style={{ fontSize: "13px", fontWeight: 600, color: "#f8fafc" }}>
            {nodeData.label}
          </div>
          <div style={{ display: "flex", alignItems: "center", gap: "6px", marginTop: "2px" }}>
            <span
              className={`text-[9px] font-mono px-1.5 py-0.2 rounded border ${badgeClass}`}
            >
              {nodeData.type}
            </span>
            {nodeData.type === "SERVICE" && (
              <span style={{ fontSize: "10px", color: "#94a3b8", fontFamily: "monospace" }}>
                {nodeData.processingDelayUs / 1000}ms
              </span>
            )}
          </div>
        </div>
      </div>

      {/* Output source handle on Right */}
      <Handle type="source" position={Position.Right} style={{ right: -4 }} />
    </div>
  );
});

CustomNode.displayName = "CustomNode";