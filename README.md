# proxy_all_classes
proxy_all_classes是一个通过注解实现AOP的扩展

## 环境要求
- PHP8.0+
- Mac or linux

## 配置
```shell
[proxy_all_classesl]
extension=proxy_all_classes
#注解
proxy.attributes=app\Attributes\TranslationAttribute
#自定义的代理类
proxy.class_name=App\Service\TransactionalServer
#自定义的代理方法
proxy.method=transaction
```
### 代码示例(通过注解实现方法事务)

- 注解类
```php
<?php

namespace App\Attributes;

#[\Attribute(\Attribute::TARGET_METHOD)]
class TranslationAttribute
{
    
}
```

- php使用
```php
<?php
namespace App\Service;

use App\Attributes\TranslationAttribute;
use Illuminate\Support\Facades\DB;

class TestService
{


    #[TranslationAttribute]
    public function test($a, $b)
    {
        
        DB::table('test')->where('id',1)->update([
            'name'=> 'testing partitions '.$a
        ]);
        
        //测试报错 事务是否回滚
        //throw new \Exception('error');

        DB::table('test')->where('id',1)->update([
            'name'=> 'testing partitions '.$b
        ]);
        
        return compact('a','b');
    }
}
```

- php代理类
```php
<?php

namespace App\Service;

use Illuminate\Support\Facades\DB;

class TransactionalServer
{

    // className  method 属性的值会在扩展中赋值
    public string $className;
    public string $method;

    /**
     * @param ...$param 原方法的参数
     * @return mixed
     * @throws \Throwable
     */
    public function transaction(...$param)
    {
        //需要代理的类名
        $className = $this->className;
        //需要代理的方法名
        $method = $this->method;
    
        return DB::connection('mysql')->transaction(function () use ($className, $method, $param) {
            return app($className)->$method(...$param);
        });
    }
}
```