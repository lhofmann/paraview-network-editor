#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_GRAPH_LAYOUT_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_GRAPH_LAYOUT_H_

#include <vector>
#include <map>

namespace ParaViewNetworkEditor {

std::map<size_t, std::pair<float, float>> compute_graph_layout(
    const std::vector<size_t> &nodes,
    const std::vector<std::pair<size_t, size_t>> &edges);

}

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_GRAPH_LAYOUT_H_
