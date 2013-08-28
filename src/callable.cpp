/**
 *  Callable.cpp
 *
 *  Implementation for the Callable class
 *
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2013 Copernica BV
 */
#include "includes.h"

/**
 *  Namespace
 */
namespace PhpCpp {

/**
 *  Function that is called by the Zend engine every time that a function gets called
 *  @param  ht      
 *  @param  return_value
 *  @param  return_value_ptr
 *  @param  this_ptr
 *  @param  return_value_used
 *  @param  tsrm_ls
 *  @return integer
 */
void invoke_callable(INTERNAL_FUNCTION_PARAMETERS)
{
    // find the function name
    const char *function = get_active_function_name(TSRMLS_C);
    
    // when we registered the function name, we have hidden the pointer to the 
    // callable right in front of the function name - we retrieve it back
    Callable *callable = *((Callable **)(function - sizeof(Callable *)));

    // call the appropriate object
    callable->invoke(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

/**
 *  Fill a function entry
 * 
 *  This method is called when the extension is registering itself, when the 
 *  function or method introces himself
 * 
 *  @param  entry
 */
void Callable::fill(zend_function_entry *entry)
{
    // fill the members of the entity
    entry->fname = _name;
    entry->handler = invoke_callable;
    entry->arg_info = _argv;
    entry->num_args = _argc;
    entry->flags = _flags;
}

/**
 *  Another attempt to fill internal function info
 * 
 *  This method is called when the extension is registering itself, when the 
 *  function or method introces himself
 * 
 *  @param  entry
 */
void Callable::fill(zend_internal_function_info *info)
{
    // fill in all the members, not that the returning by reference is not used
    info->_name = _name;
    info->_name_len = _data.size() - sizeof(this);
    info->_class_name = _classname.size() ? _classname.c_str() : NULL;
    info->required_num_args = _required;
    info->_type_hint = _type;
    info->return_reference = false;
    info->pass_rest_by_reference = false;
}

/**
 *  Process the arguments
 * 
 *  The arguments are called by the user of the PhpCpp library when he 
 * 
 *  @param  arguments
 */
void Callable::process(const std::initializer_list<Argument> &arguments)
{
    // store counters
    _argc = arguments.size();
    _required = arguments.size();
    
    // allocate memory for the arguments, with one extra record to hold information
    _argv = new zend_arg_info[_argc + 1];
    
    // fill the info
    fill((zend_internal_function_info *)_argv);
    
    // iteration counter
    int i = 0;
    
    // loop through the arguments
    for (auto it = begin(arguments); it != arguments.end(); it++)
    {
        // fill the argument structure
        it->internal()->fill(&_argv[++i]);
    }
}

/**
 *  Invoke the method
 *  @param  ht      
 *  @param  return_value
 *  @param  return_value_ptr
 *  @param  this_ptr
 *  @param  return_value_used
 *  @param  tsrm_ls
 *  @return integer
 */
int Callable::invoke(INTERNAL_FUNCTION_PARAMETERS)
{
    
    
    
}

/**
 *  End of namespace
 */
}

