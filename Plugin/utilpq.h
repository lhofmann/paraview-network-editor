#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_UTILPQ_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_UTILPQ_H_

#include <QColor>
#include <string>
#include <vector>

class pqPipelineSource;
class pqPipelineFilter;
class vtkSMParaViewPipelineControllerWithRendering;
class pqView;

namespace ParaViewNetworkEditor {
namespace utilpq {

bool multiple_inputs(pqPipelineFilter *filter, int port);

bool optional_input(pqPipelineFilter *filter, int port);

bool can_connect(pqPipelineSource *source, int out_port, pqPipelineFilter *dest, int in_port);

void add_connection(pqPipelineSource *source, int out_port, pqPipelineFilter *dest, int in_port);

void remove_connection(pqPipelineSource *source, int out_port, pqPipelineFilter *dest, int in_port);

void clear_connections(pqPipelineFilter *filter, int port);

std::vector<pqPipelineSource *> get_sources();

std::vector<pqView*> get_views();

std::pair<bool, bool> output_visibiility(pqPipelineSource *source, int out_port);

void toggle_output_visibility(pqPipelineSource *source, int out_port);

void toggle_source_visibility(pqPipelineSource *source);

void set_source_visiblity(pqPipelineSource *source, bool visible);

void set_source_scalar_bar_visiblity(pqPipelineSource *source, bool visible);

std::vector<std::string> input_datatypes(pqPipelineFilter *filter, int in_port);

const QColor default_color(188, 188, 188);

QColor output_dataset_color(pqPipelineSource *filter, int port);

}
}

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_UTILPQ_H_
