import { create } from "zustand";
import {
  Node,
  Edge,
  OnNodesChange,
  OnEdgesChange,
  OnConnect,
  applyNodeChanges,
  applyEdgeChanges,
  addEdge,
  Connection,
} from "@xyflow/react";

export interface NodeData {
  label: string;
  type: "CLIENT" | "SERVICE" | "ROUTER" | "LOAD_BALANCER";
  processingDelayUs: number;
  maxConcurrency: number;
  queueCapacity: number;
}

export interface EdgeData {
  propagationDelayUs: number;
  lossProbability: number;
}

interface TopologyState {
  nodes: Node[];
  edges: Edge[];
  selectedElement: { id: string; type: "node" | "edge" } | null;
  onNodesChange: OnNodesChange;
  onEdgesChange: OnEdgesChange;
  onConnect: OnConnect;
  setSelectedElement: (elem: { id: string; type: "node" | "edge" } | null) => void;
  addNode: (type: NodeData["type"]) => void;
  deleteSelected: () => void;
  updateNodeData: (id: string, partial: Partial<NodeData>) => void;
  updateEdgeData: (id: string, latencyUs: number, lossProb: number) => void;
  exportScenarioPayload: () => Record<string, unknown>;
  resetToTemplate: () => void;
}

const initialNodes: Node[] = [
  {
    id: "1",
    type: "custom",
    position: { x: 120, y: 180 },
    data: {
      label: "Client-1",
      type: "CLIENT",
      processingDelayUs: 0,
      maxConcurrency: 1,
      queueCapacity: 100,
    },
  },
  {
    id: "2",
    type: "custom",
    position: { x: 520, y: 180 },
    data: {
      label: "Backend-Service",
      type: "SERVICE",
      processingDelayUs: 5000,
      maxConcurrency: 4,
      queueCapacity: 20,
    },
  },
];

const initialEdges: Edge[] = [
  {
    id: "e1-2",
    source: "1",
    target: "2",
    data: { propagationDelayUs: 10000, lossProbability: 0.0 },
    animated: true,
  },
];

let nodeIdCounter = 10;

export const useTopologyStore = create<TopologyState>((set, get) => ({
  nodes: initialNodes,
  edges: initialEdges,
  selectedElement: null,

  onNodesChange: (changes) =>
    set({
      nodes: applyNodeChanges(changes, get().nodes),
    }),

  onEdgesChange: (changes) =>
    set({
      edges: applyEdgeChanges(changes, get().edges),
    }),

  onConnect: (connection: Connection) =>
    set({
      edges: addEdge(
        {
          ...connection,
          data: { propagationDelayUs: 5000, lossProbability: 0.0 },
          animated: true,
        },
        get().edges
      ),
    }),

  setSelectedElement: (elem) => set({ selectedElement: elem }),

  addNode: (type) => {
    const nextId = String(nodeIdCounter++);
    const newNode: Node = {
      id: nextId,
      type: "custom",
      position: { x: 160, y: 120 + (get().nodes.length * 80) % 360 },
      data: {
        label: `${type === "LOAD_BALANCER" ? "LB" : type}-${nextId}`,
        type,
        processingDelayUs: type === "SERVICE" ? 4000 : 0,
        maxConcurrency: 4,
        queueCapacity: 20,
      },
    };
    set({ nodes: [...get().nodes, newNode] });
  },

  deleteSelected: () => {
    const { selectedElement, nodes, edges } = get();
    if (!selectedElement) return;

    if (selectedElement.type === "node") {
      set({
        nodes: nodes.filter((n) => n.id !== selectedElement.id),
        edges: edges.filter(
          (e) => e.source !== selectedElement.id && e.target !== selectedElement.id
        ),
        selectedElement: null,
      });
    } else {
      set({
        edges: edges.filter((e) => e.id !== selectedElement.id),
        selectedElement: null,
      });
    }
  },

  updateNodeData: (id, partial) => {
    set({
      nodes: get().nodes.map((node) => {
        if (node.id === id) {
          const currentData = (node.data ?? {}) as Record<string, unknown>;
          return {
            ...node,
            data: { ...currentData, ...partial },
          };
        }
        return node;
      }),
    });
  },

  updateEdgeData: (id, latencyUs, lossProb) => {
    set({
      edges: get().edges.map((edge) => {
        if (edge.id === id) {
          const currentData = (edge.data ?? {}) as Record<string, unknown>;
          return {
            ...edge,
            data: {
              ...currentData,
              propagationDelayUs: latencyUs,
              lossProbability: lossProb,
            },
          };
        }
        return edge;
      }),
    });
  },

  exportScenarioPayload: () => {
    const { nodes, edges } = get();

    // Map each edge whose source is a CLIENT into an active workload
    const clientNodeIds = new Set(
      nodes
        .filter((n) => ((n.data ?? {}) as unknown as NodeData).type === "CLIENT")
        .map((n) => n.id)
    );

    const workloads = edges
      .filter((e) => clientNodeIds.has(e.source))
      .map((e) => ({
        source_id: parseInt(e.source, 10),
        target_id: parseInt(e.target, 10),
        arrival_rate_rps: 10.0,
        duration_us: 3000000,
        timeout_us: 1000000,
      }));

    // Fallback if no client-to-service links exist
    if (workloads.length === 0 && edges.length > 0) {
      workloads.push({
        source_id: parseInt(edges[0].source, 10),
        target_id: parseInt(edges[0].target, 10),
        arrival_rate_rps: 10.0,
        duration_us: 3000000,
        timeout_us: 1000000,
      });
    }

    return {
      config: {
        seed: 42,
        max_duration_us: 3000000,
        max_events: 100000,
      },
      nodes: nodes.map((n) => {
        const d = (n.data ?? {}) as unknown as NodeData;
        return {
          id: parseInt(n.id, 10) || 1,
          name: d.label || `Node-${n.id}`,
          type: d.type || "SERVICE",
          processing_delay_us: d.processingDelayUs || 0,
          max_concurrency: d.maxConcurrency || 1,
          queue_capacity: d.queueCapacity || 50,
        };
      }),
      links: edges.map((e, idx) => {
        const ed = (e.data ?? {}) as unknown as EdgeData;
        return {
          id: idx + 1,
          source_id: parseInt(e.source, 10),
          target_id: parseInt(e.target, 10),
          propagation_delay_us: ed.propagationDelayUs ?? 5000,
          bandwidth_bps: 10000000,
          loss_probability: ed.lossProbability ?? 0.0,
        };
      }),
      workloads,
      faults: [],
    };
  },

  resetToTemplate: () => {
    nodeIdCounter = 10;
    set({ nodes: initialNodes, edges: initialEdges, selectedElement: null });
  },
}));