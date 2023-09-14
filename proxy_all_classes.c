/* proxy_all_classes extension for PHP */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "ext/standard/php_var.h"
#include "php_proxy_all_classes.h"
#include "proxy_all_classes_arginfo.h"
#include "zend_attributes.h"
#include "zend_types.h"
#include "zend_API.h"
#include "ext/pdo/php_pdo.h"
#include "ext/pdo/php_pdo_driver.h"

/* For compatibility with older PHP versions */
#ifndef ZEND_PARSE_PARAMETERS_NONE
#define ZEND_PARSE_PARAMETERS_NONE()  \
	ZEND_PARSE_PARAMETERS_START(0, 0) \
	ZEND_PARSE_PARAMETERS_END()
#endif

static void (*original_zend_execute_ex)(zend_execute_data *execute_data);

void my_execute_ex(zend_execute_data *execute_data)
{

	if (execute_data->func->common.attributes == NULL)
	{
		original_zend_execute_ex(execute_data);
		return;
	}

	if (zend_get_attribute_str(execute_data->func->common.attributes, ZEND_STRL("app\\attributes\\translationattribute")) == NULL)
	{
		original_zend_execute_ex(execute_data);
		return;
	}

	if (execute_data->func == NULL)
	{
		original_zend_execute_ex(execute_data);
		return;
	}

	char *proxy_class_name = "App\\Service\\TransactionalServer";
	zend_string *class_name = zend_string_init(proxy_class_name, strlen(proxy_class_name), 0);
	zend_class_entry *ce = zend_fetch_class(class_name, ZEND_FETCH_CLASS_AUTO);
	if (ce == NULL)
	{
		php_error_docref(NULL, E_WARNING, "Class %s not found", proxy_class_name);
		return;
	}

	zend_function *fbc = zend_hash_find_ptr(&ce->function_table, zend_string_init(ZEND_STRL("transaction"), 0));
	if (fbc == NULL)
	{
		original_zend_execute_ex(execute_data);
		return;
	}
	
	zend_declare_property_string(ce, "className", strlen("className"), execute_data->func->common.scope->name->val, ZEND_ACC_PUBLIC);
	zend_declare_property_string(ce, "method", strlen("method"), execute_data->func->common.function_name->val, ZEND_ACC_PUBLIC);
	
	zval obj;
	object_init_ex(&obj,ce);

	zval *ret = execute_data->return_value;

	uint32_t param_count = ZEND_CALL_NUM_ARGS(execute_data);
	zend_execute_data *call = zend_vm_stack_push_call_frame(ZEND_CALL_TOP_FUNCTION, fbc, param_count, ce);

	for (uint32_t i = 0; i < param_count; i++)
	{
		ZVAL_COPY(ZEND_CALL_ARG(call, i + 1), ZEND_CALL_VAR_NUM(execute_data, i));
	}
	call->This = obj;

	call->prev_execute_data = execute_data;
	zend_init_execute_data(call, (zend_op_array *)fbc, ret);
	zend_vm_stack_free_call_frame(execute_data);
	zend_vm_stack_free_call_frame(call);
	original_zend_execute_ex(call);
}
/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(proxy_all_classes)
{
#if defined(ZTS) && defined(COMPILE_DL_PROXY_ALL_CLASSES)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	original_zend_execute_ex = zend_execute_ex;
	zend_execute_ex = my_execute_ex;

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(proxy_all_classes)
{
	zend_execute_ex = original_zend_execute_ex;

	return SUCCESS;
}

/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(proxy_all_classes)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "proxy_all_classes support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ proxy_all_classes_module_entry */
zend_module_entry proxy_all_classes_module_entry = {
	STANDARD_MODULE_HEADER,
	"proxy_all_classes",			  /* Extension name */
	ext_functions,					  /* zend_function_entry */
	NULL,							  /* PHP_MINIT - Module initialization */
	PHP_MSHUTDOWN(proxy_all_classes), /* PHP_MSHUTDOWN - Module shutdown */
	PHP_RINIT(proxy_all_classes),	  /* PHP_RINIT - Request initialization */
	NULL,							  /* PHP_RSHUTDOWN - Request shutdown */
	PHP_MINFO(proxy_all_classes),	  /* PHP_MINFO - Module info */
	PHP_PROXY_ALL_CLASSES_VERSION,	  /* Version */
	STANDARD_MODULE_PROPERTIES};
/* }}} */

#ifdef COMPILE_DL_PROXY_ALL_CLASSES
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(proxy_all_classes)
#endif
