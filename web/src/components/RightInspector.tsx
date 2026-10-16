"use client";

import React from "react";
import { Trash2 } from "lucide-react";
import { useTopologyStore, NodeData, EdgeData } from "@/store/topologyStore";

export const RightInspector: React.FC = () => {
  const {
    nodes,
    edges,
    selectedElement,
    deleteSelected,
    updateNodeData,
    updateEdgeData,
  } = useTopologyStore();

  if (!selectedElement) {
    return (
      <aside className="gn-inspector" style={{ alignItems: "center", justifyContent: "center", textAlign: "center" }}>
        <div style={{ width: "36px", height: "36px", borderRadius: "50%", background: "#151d30", border: "1px solid #243048", color: "#06b6d4", display: "flex", alignItems: "center", justifyContent: "center", marginBottom: "8px", fontSize: "16px" }}>
          ⚙
        </div>
        <div style={{ fontSize: "12px", color: "#64748b" }}>
          Select a node or link to configure properties
        </div>
      </aside>
    );
  }

  if (selectedElement.type === "node") {
    const node = nodes.find((n) => n.id === selectedElement.id);
    if (!node) return null;
    const data = (node.data ?? {}) as unknown as NodeData;
    const isService = data.type === "SERVICE";

    return (
      <aside className="gn-inspector">
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "flex-start", borderBottom: "1px solid #1e293b", paddingBottom: "10px", marginBottom: "14px" }}>
          <div>
            <div style={{ fontSize: "11px", fontWeight: 700, textTransform: "uppercase", letterSpacing: "1px", color: "#06b6d4" }}>
              Node Inspector
            </div>
            <div style={{ fontSize: "11px", color: "#64748b", marginTop: "2px" }}>
              ID: {node.id} ({data.type})
            </div>
          </div>
          <button
            onClick={deleteSelected}
            title="Delete Node"
            style={{
              padding: "4px 8px",
              background: "rgba(244, 63, 94, 0.1)",
              border: "1px solid rgba(244, 63, 94, 0.3)",
              color: "#f43f5e",
              borderRadius: "4px",
              cursor: "pointer",
            }}
          >
            <Trash2 style={{ width: "13px", height: "13px" }} />
          </button>
        </div>

        <div style={{ display: "flex", flexDirection: "column", gap: "12px", fontSize: "12px" }}>
          <div>
            <label style={{ display: "block", color: "#94a3b8", marginBottom: "4px" }}>Node Label</label>
            <input
              type="text"
              value={data.label || ""}
              onChange={(e) => updateNodeData(node.id, { label: e.target.value })}
              className="gn-input"
            />
          </div>

          {isService ? (
            <>
              <div>
                <label style={{ display: "block", color: "#94a3b8", marginBottom: "4px" }}>Processing Delay (μs)</label>
                <input
                  type="number"
                  value={data.processingDelayUs || 0}
                  onChange={(e) =>
                    updateNodeData(node.id, {
                      processingDelayUs: parseInt(e.target.value, 10) || 0,
                    })
                  }
                  className="gn-input"
                />
              </div>

              <div>
                <label style={{ display: "block", color: "#94a3b8", marginBottom: "4px" }}>Max Concurrency (Workers)</label>
                <input
                  type="number"
                  min="1"
                  value={data.maxConcurrency || 1}
                  onChange={(e) =>
                    updateNodeData(node.id, {
                      maxConcurrency: parseInt(e.target.value, 10) || 1,
                    })
                  }
                  className="gn-input"
                />
              </div>

              <div>
                <label style={{ display: "block", color: "#94a3b8", marginBottom: "4px" }}>Queue Capacity</label>
                <input
                  type="number"
                  min="1"
                  value={data.queueCapacity || 10}
                  onChange={(e) =>
                    updateNodeData(node.id, {
                      queueCapacity: parseInt(e.target.value, 10) || 1,
                    })
                  }
                  className="gn-input"
                />
              </div>
            </>
          ) : (
            <div style={{ padding: "10px", borderRadius: "6px", background: "#151d30", border: "1px solid #1e293b", color: "#64748b", fontSize: "11px", lineHeight: "1.4" }}>
              This node acts as a virtual traffic generator and sends requests over connected links[cite: 1].
            </div>
          )}
        </div>
      </aside>
    );
  }

  // Edge inspector
  const edge = edges.find((e) => e.id === selectedElement.id);
  if (!edge) return null;
  const edgeData = (edge.data ?? {}) as unknown as EdgeData;

  return (
    <aside className="gn-inspector">
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "flex-start", borderBottom: "1px solid #1e293b", paddingBottom: "10px", marginBottom: "14px" }}>
        <div>
          <div style={{ fontSize: "11px", fontWeight: 700, textTransform: "uppercase", letterSpacing: "1px", color: "#06b6d4" }}>
            Link Inspector
          </div>
          <div style={{ fontSize: "11px", color: "#64748b", marginTop: "2px" }}>
            Hop: {edge.source} → {edge.target}
          </div>
        </div>
        <button
          onClick={deleteSelected}
          title="Delete Link"
          style={{
            padding: "4px 8px",
            background: "rgba(244, 63, 94, 0.1)",
            border: "1px solid rgba(244, 63, 94, 0.3)",
            color: "#f43f5e",
            borderRadius: "4px",
            cursor: "pointer",
          }}
        >
          <Trash2 style={{ width: "13px", height: "13px" }} />
        </button>
      </div>

      <div style={{ display: "flex", flexDirection: "column", gap: "12px", fontSize: "12px" }}>
        <div>
          <label style={{ display: "block", color: "#94a3b8", marginBottom: "4px" }}>Propagation Delay (μs)</label>
          <input
            type="number"
            value={edgeData.propagationDelayUs ?? 5000}
            onChange={(e) =>
              updateEdgeData(
                edge.id,
                parseInt(e.target.value, 10) || 0,
                edgeData.lossProbability ?? 0.0
              )
            }
            className="gn-input"
          />
        </div>

        <div>
          <label style={{ display: "block", color: "#94a3b8", marginBottom: "4px" }}>Packet Loss (0.0 to 1.0)</label>
          <input
            type="number"
            step="0.05"
            min="0"
            max="1"
            value={edgeData.lossProbability ?? 0.0}
            onChange={(e) =>
              updateEdgeData(
                edge.id,
                edgeData.propagationDelayUs ?? 5000,
                parseFloat(e.target.value) || 0.0
              )
            }
            className="gn-input"
          />
        </div>
      </div>
    </aside>
  );
};