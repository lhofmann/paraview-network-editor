#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_UTILPQ_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_UTILPQ_H_

class pqPipelineSource;
class pqPipelineFilter;

namespace utilpq {

bool multiple_inputs(pqPipelineFilter* filter, int port);

bool optional_input(pqPipelineFilter* filter, int port);

bool can_connect(pqPipelineSource* source, int out_port, pqPipelineFilter* dest, int in_port);

void add_connection(pqPipelineSource* source, int out_port, pqPipelineFilter* dest, int in_port);

void remove_connection(pqPipelineSource* source, int out_port, pqPipelineFilter* dest, int in_port);

void clear_connections(pqPipelineFilter* filter, int port);

}

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_UTILPQ_H_
