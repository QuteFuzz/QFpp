#include <iostream>
#include <sstream>
#include <string>

#include <resource.h>

std::string Resource::resolved_name() const {

    if(is_register_def()){
        return get_name()->get_content() + "[" + get_index()->get_content() + "]";
    } else {
        return get_name()->get_content();
    }

}

