#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_DEBUG_MESSAGE_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_DEBUG_MESSAGE_H_

#include <iostream>

#ifndef NDEBUG
# define DEBUG_MSG(str) do { std::cout << str << std::endl; } while( false )
#else
# define DEBUG_MSG(str) do { } while ( false )
#endif

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_DEBUG_MESSAGE_H_
