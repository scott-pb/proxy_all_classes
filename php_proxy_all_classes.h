/* proxy_all_classes extension for PHP */

#ifndef PHP_PROXY_ALL_CLASSES_H
# define PHP_PROXY_ALL_CLASSES_H

extern zend_module_entry proxy_all_classes_module_entry;
# define phpext_proxy_all_classes_ptr &proxy_all_classes_module_entry

# define PHP_PROXY_ALL_CLASSES_VERSION "0.1.0"

# if defined(ZTS) && defined(COMPILE_DL_PROXY_ALL_CLASSES)
ZEND_TSRMLS_CACHE_EXTERN()
# endif

#endif	/* PHP_PROXY_ALL_CLASSES_H */
