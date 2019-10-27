#include "graph_layout.h"

#include <graphviz/cgraph.h>
#include <graphviz/gvc.h>
#include <string>

std::map<size_t, std::pair<float, float>> compute_graph_layout(
    const std::vector<size_t>& nodes,
    const std::vector<std::pair<size_t, size_t>>& edges
)
{
  std::string dot;

  dot = R"DOT(
  digraph {
    graph [dpi=50 nodesep=0.01 ranksep=0.1]
    node [fixedsize=true height=1 shape=box width=2.5]
  )DOT";

  for (size_t i : nodes) {
    dot += std::to_string(i) + "\n";
  }
  for (const auto& e : edges) {
    dot += std::to_string(e.first) + " -> " + std::to_string(e.second) + "\n";
  }

  dot += "}\n";

  Agraph_t *G = agmemread(dot.data());
  GVC_t *gvc = gvContext();
  gvLayout(gvc, G, "dot");

  std::map<size_t, std::pair<float, float>> result;

  for (size_t i : nodes) {
    Agnode_t* node = agnode(G, const_cast<char *>(std::to_string(i).data()), 0);
    if (node) {
      auto &coord = ND_coord(node);
      result[i] = std::make_pair(coord.x, coord.y);
    }
  }

  gvFreeLayout(gvc, G);
  agclose(G);
  gvFreeContext(gvc);

  return result;
}
