## 项目说明
基于flex和bison实现的一个基本的脚本解释器，有如下一些特点：  
1. 支持变量作用域和生命周期自动化管理。  
2. 支持资源的生命周期管理，例如文件句柄生命周期。    
3. 解析和执行分开，解析的中间结果可以方便序列化和反序列化。  
## 基本语法
值类型  
string - 字符串  
integer- 整数  
float  - 双精度浮点  
array  - 数组（数组可存储所有值类型）  
map    - 字典  
NULL   - NULL值  
Resource - 系统资源，例如文件句柄  

变量定义  
```
var name1,name2,name3="1234";
这里name1,name2被初始化成NULL值
```
赋值时自动定义  
```
name4 = 128;这种情况，只有上下文中中不存在name4变量的时候才会创建变量，如果存在就是赋值语句  
``` 
变量作用域  
变量有3种作用域 文件作用域 函数作用域 局部作用域（for，for-in switch 语句） 
离开作用域，变量自动销毁  
```
var name5; #文件作用域
func do_something(a,b){
    var name5; #顶层的name5被屏蔽
    for(var i =0;i < 10;i++){
        var name5;#顶层的name5、函数内变量name5被屏蔽
    }
}
```
语法  
支持大部分现代脚本的控制结构  
赋值 = += -= *= /=   
基本算术运算 + - * / %(整数求余)   
条件判断 if else 语法  
循环语法 for      
switch语法  
数组和字典迭代 for ... in ...  
数组切片  a=b[2:] ;a=b[1:4];a=b[:10];  
支持自定义函数、内置函数  

## 语法示例  
```
var _test_pass = true;

func assertEqual(a,b){
    if(a != b){
        Println("error here a=",a,"b=",b);
        _test_pass = false;
    }
}

func assertNotEqual(a,b){
    if(a == b){
        Println("error here a=",a,"b=",b);
        _test_pass = false;
    } 
}

Println("test for (break,continue) statement");

var _execute_count =0;
var _not_executed  =0;

for (var i =0;i< 9;i++){
    _execute_count ++;
    if (i>5){
        continue;
    }
    _not_executed ++;
}

assertEqual(_execute_count,9);
assertEqual(_not_executed,6);

_execute_count = 0;
_not_executed = 0;
var _index = 0;
for (;_index< 9;_index++){
    _execute_count ++;
    if (_index>5){
        continue;
    }
    _not_executed ++;
}

_execute_count = 0;
_not_executed = 0;
_index = 0;
for (;;_index++){
    _execute_count ++;
    if(_index > 8){
        break;
    }
    _not_executed ++;
}
assertEqual(_execute_count,10);
assertEqual(_not_executed,9);

_execute_count = 0;
_not_executed = 0;
_index = 0;
for {
    _execute_count++;
    if(_index > 8){
        break;
    }
    _not_executed++;
    _index ++;
}
assertEqual(_execute_count,10);
assertEqual(_not_executed,9);

Println("test arithmetic operation");
assertEqual(1*(2+3),5);
assertEqual(1+1-1,1);
assertEqual(1+2*3,7);

Println("test shadow name");
var name1 = 0;
for (var name1 = 0; name1 < 6;name1++ ){
}
assertEqual(name1,0);

var count = 0;
var count2 = 0;
var count3 = 0,count4 =0;

func find_number(max){
    for(var i = 1; i< max;i++){
        if(i%2 == 0){
            count++;
        }else if(i%3 ==0){
            count2++;
        }else if(i%5 ==0){
            count3++;
        }else if(i%4 == 0){
            count4++;
        }else{

        }
    }
}
assertEqual(count4,0);
find_number(100);
Println(count,count2,count3);
var type = TypeOf(count);
Println(type);

#array & map
var array_item = "string";
var list = ["value1","value2",100,array_item];

var sub_list = list[1:];

assertEqual(len(sub_list),3);
assertEqual(sub_list[0],"value2");

for v in list{
    Println(v);
}

assertEqual(list[0],"value1");
assertEqual(len(list[0]),6);
assertEqual(len(list),4);

var dic = {"type":"cat","age":2};
assertEqual(dic["type"],"cat");
assertEqual(dic["age"],2);
dic["name"] = "wawa";
assertEqual(dic["name"],"wawa");
for k,v in dic{
    Println(ToString(k)+":"+ToString(v));
}

var string_count = 0;
func printValueType(val){
    switch(TypeOf(val)){
        case "String":{
            Println(val,"is a string");
            break;
            string_count++;
        }
        case "Integer":{
            Println(val,"is a Integer");
        }
        case "Float":{
            Println(val,"is a Float");
        }
        default:{
            Println("other...");
        }
    }
}
assertEqual(string_count,0);
printValueType("100");
printValueType(100);
printValueType(3.1415926);
var nullobj;
printValueType(nullobj);

var _dirs = ["/bin/","/usr/bin/","/usr/local/bin/"];
var _files = ["test1","test2","test3"];
var _full_paths = [];
var full;
for v in _dirs{
    for v2 in _files{
        _full_paths = append(_full_paths, v+v2);
    }
}
assertEqual(len(_full_paths),len(_dirs)*len(_files));
Println(_full_paths);
```
## 参考
https://github.com/stdpain/compiler-interpreter
