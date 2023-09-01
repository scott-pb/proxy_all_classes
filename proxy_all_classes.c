/* proxy_all_classes extension for PHP */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
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
static void (*original_zend_execute_internal)(zend_execute_data *execute_data, zval *return_value);

void my_execute_internal(zend_execute_data *execute_data, zval *return_value);
void my_execute_ex(zend_execute_data *execute_data);

void my_execute_internal(zend_execute_data *execute_data, zval *return_value)
{
	original_zend_execute_internal(execute_data, return_value);
}
static zif_handler original_handler = NULL;

static ZEND_NAMED_FUNCTION(my_proxy_function)
{
	php_printf("%s\n", "my_proxy_function");
	// original_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

void my_execute_ex(zend_execute_data *execute_data)
{

	if (execute_data->func->common.attributes != NULL)
	{

		zend_attribute *attr = zend_get_attribute_str(execute_data->func->common.attributes, ZEND_STRL("app\\attributes\\translationattribute"));
		if (attr != NULL && execute_data->func != NULL)
		{
			zval *param;
			HashTable param_array;
			zend_hash_init(&param_array, 0, NULL, NULL, 0);

			for (uint32_t i = 0; i < execute_data->func->common.num_args; i++)
			{
				param = ZEND_CALL_VAR_NUM(execute_data, i);
				zend_hash_next_index_insert(&param_array, param);
			}

			// zend_string *class_name = execute_data->func->common.scope->name;
			// php_printf("Current Class Name: %s\n", ZSTR_VAL(class_name));

			char *proxy_class_name = "App\\Services\\TransactionalServer";

			zend_class_entry *ce = zend_fetch_class(zend_string_init(proxy_class_name, strlen(proxy_class_name), 0), ZEND_FETCH_CLASS_AUTO);
			if (ce == NULL)
			{
				php_error_docref(NULL, E_WARNING, "Class %s not found", proxy_class_name);
				return;
			}

			zval obj;
			object_init_ex(&obj, ce);
			zval retval;

			// zend_class_entry myext_interface_def;
			// INIT_CLASS_ENTRY_EX()

			// //zend_class_entry *ce = (zend_class_entry *)Z_PTR_P(&execute_data->This);
			// php_printf("name:%s\n",  execute_data->func->common.scope);
			// // zend_class_entry new_class_ce;

			// execute_data->call;
			// original_handler = original->internal_function.handler;

			// original->internal_function.handler = my_proxy_function;

			// return;

			// zend_class_entry *target_ce = zend_lookup_class(target_class_name);
			// php_printf("target_class_name:%s\n", ZSTR_VAL(target_class_name));
			// zend_class_entry proxy_ce;
			// memcpy(&proxy_ce, target_ce, sizeof(zend_class_entry));

			// zend_string *proxy_class_name = zend_string_alloc(ZSTR_LEN(target_class_name) + sizeof("_Proxy") - 1, 0);
			// snprintf(ZSTR_VAL(proxy_class_name), ZSTR_LEN(proxy_class_name), "%s%s", ZSTR_VAL(target_class_name), "_Proxy");
			// php_printf("proxy_class_name:%s\n", proxy_class_name->val);
			// zend_result result = zend_register_class_alias_ex(ZSTR_VAL(proxy_class_name), ZSTR_LEN(proxy_class_name), &proxy_ce, 0);
			// php_printf("result:%d\n", result);
			// zend_string_release(proxy_class_name);
		}
	}

	original_zend_execute_ex(execute_data);
}
/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(proxy_all_classes)
{
#if defined(ZTS) && defined(COMPILE_DL_PROXY_ALL_CLASSES)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	original_zend_execute_internal = zend_execute_internal;
	zend_execute_internal = my_execute_internal;

	original_zend_execute_ex = zend_execute_ex;
	zend_execute_ex = my_execute_ex;

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(proxy_all_classes)
{
	zend_execute_internal = original_zend_execute_internal;
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
