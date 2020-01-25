function(alias_pkg_config_target newTarget targetToClone)
    add_library(${newTarget} INTERFACE IMPORTED)
    foreach(name INTERFACE_LINK_LIBRARIES INTERFACE_INCLUDE_DIRECTORIES INTERFACE_COMPILE_DEFINITIONS INTERFACE_COMPILE_OPTIONS)
        get_property(value TARGET ${targetToClone} PROPERTY ${name})
        set_property(TARGET ${newTarget} PROPERTY ${name} ${value})
    endforeach()
endfunction()
