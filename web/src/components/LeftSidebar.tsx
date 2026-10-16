"use client";

import React from "react";
import { Server, Monitor, Cpu, Split } from "lucide-react";
import { useTopologyStore, NodeData } from "@/store/topologyStore";

export const LeftSidebar: React.FC = () => {
  const addNode = useTopologyStore((state) => state.addNode);

  const nodePalette: { type: NodeData["type"]; label: string; sub: string; icon: any }[] = [
    { type: "CLIENT", label: "Client Generator", sub: "Traffic source", icon: Monitor },
    { type: "SERVICE", label: "Backend Service", sub: "Processing node", icon: Server },
    { type: "ROUTER", label: "Router / Hop", sub: "Packet forwarding", icon: Cpu },
    { type: "LOAD_BALANCER", label: "Load Balancer", sub: "Traffic distributor", icon: Split },
  ];

  return (
    <aside className="gn-sidebar">
      <div style={{ marginBottom: "16px" }}>
        <div style={{ fontSize: "11px", fontWeight: 700, textTransform: "uppercase", letterSpacing: "1px", color: "#06b6d4" }}>
          Components Palette
        </div>
        <div style={{ fontSize: "11px", color: "#64748b", marginTop: "3px" }}>
          Click to add virtual node
        </div>
      </div>

      <div style={{ display: "flex", flexDirection: "column", gap: "8px" }}>
        {nodePalette.map((item) => {
          const Icon = item.icon;
          return (
            <button
              key={item.type}
              onClick={() => addNode(item.type)}
              className="gn-palette-btn"
            >
              <div
                style={{
                  padding: "6px",
                  borderRadius: "6px",
                  background: "#080c14",
                  color: "#06b6d4",
                  display: "flex",
                }}
              >
                <Icon style={{ width: "16px", height: "16px" }} />
              </div>
              <div>
                <div style={{ fontSize: "12px", fontWeight: 600, color: "#f8fafc" }}>
                  {item.label}
                </div>
                <div style={{ fontSize: "10px", color: "#64748b" }}>
                  {item.sub}
                </div>
              </div>
            </button>
          );
        })}
      </div>
    </aside>
  );
};