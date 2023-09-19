/* proxy_all_classes extension for PHP */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_var.h"
#include "php_proxy_all_classes.h"
#include "proxy_all_classes_arginfo.h"
#include "zend_attributes.h"
#include "zend_types.h"
#include "zend_API.h"

/* For compatibility with older PHP versions */
#ifndef ZEND_PARSE_PARAMETERS_NONE
#define ZEND_PARSE_PARAMETERS_NONE()  \
	ZEND_PARSE_PARAMETERS_START(0, 0) \
	ZEND_PARSE_PARAMETERS_END()
#endif

static zend_result ini_status = SUCCESS;

struct proxy_all_classes_attributes
{
	char * proxy_attributes_val;
	size_t proxy_attributes_len;
};


//ini属性结构体
ZEND_BEGIN_MODULE_GLOBALS(proxy_all_classes)
	char * proxy_attributes;
	char * proxy_class_name;
	char * proxy_method;
ZEND_END_MODULE_GLOBALS(proxy_all_classes)

//初始化
static PHP_GINIT_FUNCTION(proxy_all_classes);

ZEND_DECLARE_MODULE_GLOBALS(proxy_all_classes)


/* {{{ PHP_GINIT_FUNCTION */
static PHP_GINIT_FUNCTION(proxy_all_classes)
{
#if defined(COMPILE_DL_BCMATH) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	proxy_all_classes_globals->proxy_attributes = "";
	proxy_all_classes_globals->proxy_class_name = "";
	proxy_all_classes_globals->proxy_method = "";
}
/* }}} */


static PHP_INI_MH(OnUpdateStringNotNull)
{
	if(ini_status == FAILURE){
		return FAILURE;
	}
	if (ZSTR_LEN(new_value) <= 0) {
		zend_error(E_ERROR, "The %s ini Must be set",ZSTR_VAL(entry->name));
		ini_status = FAILURE;
		return FAILURE;
	}


	OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3, stage);
	ini_status = SUCCESS;
	return SUCCESS;
}

// name, default_value, modifiable, on_modify, property_name, struct_type, struct_ptr
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("proxy.attributes", "", PHP_INI_ALL, OnUpdateStringNotNull, proxy_attributes, zend_proxy_all_classes_globals, proxy_all_classes_globals)
	STD_PHP_INI_ENTRY("proxy.class_name", "", PHP_INI_ALL, OnUpdateStringNotNull, proxy_class_name, zend_proxy_all_classes_globals, proxy_all_classes_globals)
	STD_PHP_INI_ENTRY("proxy.method", "", PHP_INI_ALL, OnUpdateStringNotNull, proxy_method, zend_proxy_all_classes_globals, proxy_all_classes_globals)
PHP_INI_END()



static void (*original_zend_execute_ex)(zend_execute_data *execute_data);

void my_execute_ex(zend_execute_data *execute_data)
{

	if (execute_data->func == NULL)
	{
		original_zend_execute_ex(execute_data);
		return;
	}
	// 方法上有没有注解
	if (execute_data->func->common.attributes == NULL)
	{
		original_zend_execute_ex(execute_data);
		return;
	}

	zend_string *proxy_attributes =  zend_string_tolower(zend_string_init(proxy_all_classes_globals.proxy_attributes,strlen(proxy_all_classes_globals.proxy_attributes),0));

	zend_attribute *zend_proxy_attribute = zend_get_attribute_str(execute_data->func->common.attributes, ZSTR_VAL(proxy_attributes),ZSTR_LEN(proxy_attributes));
	// 获取方法上的指定注解
	if (zend_proxy_attribute == NULL)
	{
		original_zend_execute_ex(execute_data);
		return;
	}

	if (strcmp(ZSTR_VAL(execute_data->prev_execute_data->func->common.scope->name), proxy_all_classes_globals.proxy_class_name) == 0)
	{
		original_zend_execute_ex(execute_data);
		return;
	}

	

	// 代理类
	zend_string *class_name = zend_string_init(proxy_all_classes_globals.proxy_class_name, strlen(proxy_all_classes_globals.proxy_class_name), 0);
	zend_class_entry *ce = zend_lookup_class(class_name);
	if (ce == NULL)
	{
		php_error_docref(NULL, E_WARNING, "Class %s not found", proxy_all_classes_globals.proxy_class_name);
		return;
	}
	// 代理方法
	zend_function *fbc = zend_hash_find_ptr(&ce->function_table, zend_string_init(proxy_all_classes_globals.proxy_method,strlen(proxy_all_classes_globals.proxy_method),0));
	if (fbc == NULL)
	{
		php_error_docref(NULL, E_WARNING, "The method:%s of the class:%s are not defined", proxy_all_classes_globals.proxy_method,proxy_all_classes_globals.proxy_class_name);
		original_zend_execute_ex(execute_data);
		return;
	}


	char *conn_name = zend_proxy_attribute->args[0].value.value.str ? ZSTR_VAL(zend_proxy_attribute->args[0].value.value.str) : "";
	
	//php_printf("%s\n",zend_proxy_attribute->args[0].value.value.str->val);

	// 需要代理的 类名+方法名设置为代理类的属性
	zend_declare_property_string(ce, "className", strlen("className"), execute_data->func->common.scope->name->val, ZEND_ACC_PUBLIC);
	zend_declare_property_string(ce, "method", strlen("method"), execute_data->func->common.function_name->val, ZEND_ACC_PUBLIC);
	zend_declare_property_string(ce, "connName", strlen("connName"), conn_name, ZEND_ACC_PUBLIC);

	zval obj;
	object_init_ex(&obj, ce);

	zval *ret = execute_data->return_value;
	// 需要代理类的参数
	uint32_t param_count = ZEND_CALL_NUM_ARGS(execute_data);

	// fbc->common.num_args = param_count;
	//  新增一个栈 且在栈顶
	zend_execute_data *call = zend_vm_stack_push_call_frame(ZEND_CALL_TOP_FUNCTION, fbc, param_count, Z_OBJ_P(&obj));

	for (uint32_t i = 0; i < param_count; i++)
	{
		ZVAL_COPY(ZEND_CALL_VAR_NUM(call, i), ZEND_CALL_VAR_NUM(execute_data, i));
	}

	zend_init_execute_data(call, (zend_op_array *)fbc, ret);
	call->prev_execute_data = execute_data;

	original_zend_execute_ex(call);
	zend_vm_stack_free_call_frame(execute_data);
	zend_vm_stack_free_call_frame(call);
}
/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(proxy_all_classes)
{
#if defined(ZTS) && defined(COMPILE_DL_PROXY_ALL_CLASSES)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(proxy_all_classes)
{
	UNREGISTER_INI_ENTRIES();
	if(ini_status == SUCCESS){
		zend_execute_ex = original_zend_execute_ex;
	}
	
	
	return SUCCESS;
}

PHP_MINIT_FUNCTION(proxy_all_classes)
{
	REGISTER_INI_ENTRIES();
	if(ini_status == SUCCESS){
		original_zend_execute_ex = zend_execute_ex;
		zend_execute_ex = my_execute_ex;
	}

	return ini_status;
}

/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(proxy_all_classes)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "proxy_all_classes support", "enabled");
	php_info_print_table_end();
	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ proxy_all_classes_module_entry */
zend_module_entry proxy_all_classes_module_entry = {
	STANDARD_MODULE_HEADER,
	"proxy_all_classes",			  /* Extension name */
	ext_functions,					  /* zend_function_entry */
	PHP_MINIT(proxy_all_classes),	  /* PHP_MINIT - Module initialization */
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
