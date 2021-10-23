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
bytes  - 无符号字符串
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
赋值 = += -= *= /= |= &= ^= >>= <<=    
基本算术运算 + - * / %(整数求余)   
整数位运算 & | ^ ~ >> <<  
逻辑运算 || &&  > >= < <= == !=       
条件判断语句 if else 语法  
循环语法 for      
switch语法  
数组和字典迭代 for ... in ...  
数组切片  a=b[2:] ;a=b[1:4];a=b[:10];  
支持自定义函数、内置函数  

## 语法示例  
```

#test help function

var _is_test_passed = true;
func assertEqual(a,b){
    if(a != b){
        Println("error here a=",a,"b=",b);
        _is_test_passed = false;
    }
}

func assertNotEqual(a,b){
    if(a == b){
        Println("error here a=",a,"b=",b);
        _is_test_passed = false;
    } 
}

#basic arithmetic operation

assertEqual(1+1+2,4);
assertEqual(1+(1+2),4);
assertEqual(1+2*4,9);
assertEqual(4*(1+2),12);
assertEqual(4*5/5,4);

func test_basic_convert(){
    var i = 100,f = 3.1415;
    var res = i+f;
    assertEqual(typeof(i),"integer");
    assertEqual(typeof(f),"float");

    #float + integer = float
    assertEqual(typeof(res),"float");

    #integer+=float  result as Integer
    i += 4.6;
    assertEqual(typeof(i),"integer");

    #convert string to bytes
    var buf = bytes("hello");
    var buf2 = bytes("world");

    assertEqual(typeof(buf),"bytes");
    assertEqual(typeof(buf2),"bytes");

    #convert Integer array to bytes
    var buf3 = bytes(0x68,0x65,0x6C,0x6C,0x6F);
    var buf4 = bytes([0x77,0x6F,0x72,0x6C,0x64]);

    assertEqual(typeof(buf3),"bytes");
    assertEqual(typeof(buf4),"bytes");
    assertEqual(buf,buf3);
    assertEqual(buf2,buf4);

    #convert hex string to bytes
    var buf5= BytesFromHexString("68656C6C6F20776F726C64");
    var buf6= append(buf,0x20,buf4);

    assertEqual(buf5,buf6);

    #convert bytes to string 
    var strTemp = string(buf5);
    assertEqual(typeof(strTemp),"string");

    var str1 = string(0x68,0x65,0x6C,0x6C,0x6F);
    assertEqual(typeof(str1),"string");
    var str2 = string([0x77,0x6F,0x72,0x6C,0x64]);
    assertEqual(typeof(str2),"string");
    var str3 = str1 + " " +str2;
    assertEqual(str3,string(buf5));
    assertEqual(str1[0],'h');
}

test_basic_convert();

func test_bitwise_operation(){
    assertEqual(0xFFEE & 0xFF,0xEE);
    assertEqual(0xFF00 | 0xFF,0xFFFF);
    assertEqual(0xFF ^ 0x00,0xFF);
    assertEqual(0xFF54>>8,0xFF);
    assertEqual(0xFF54<<8,0xFF5400);
    assertEqual(true || false,true);
    assertEqual(false || true,true);
    assertEqual(false && true,false);
    assertEqual(true && false,false);
    assertEqual(true && true,true);
}

test_bitwise_operation();


func test_loop(){
    var j =0,k=0;
    for (var i = 0;i< 9;i++){
        j++;
        k++;
    }
    assertEqual(j,9);
    assertEqual(k,9);

    j=0;
    k=0;
    for (var i = 0;i< 9;i++){
        j++;
       if(!(i%2)){
           k++;
       }
    }
    assertEqual(k,5);
    assertEqual(j,9);
    j=0;
    k=0;
    for (var i = 0;i< 9;i++){
        j++;
        if(i > 5){
            continue;
        }
        k++;
    }
    assertEqual(k,6);
    assertEqual(j,9);

    j=0;
    k=0;
    for (var i = 0;i< 20;i++){
        j++;
        if(i >= 8){
            break;
        }
        k++;
    }
    assertEqual(k,8);
    assertEqual(j,9);

    j=0;
    k=0;
    for{
        j++;
        k++;
        if(j > 8){
            break;
        }
    }
    #empty body
    for(var i =0;i<3;i++){

    }
    assertEqual(k,9);
    assertEqual(j,9);
}

test_loop();

func test_shadow_name(name1){
    var name2 = name1;
    for(var name2 =0; name2< 3;name2++){
    }
    for(var i =0; i< 3;i++){
        name2++;
    }
    assertNotEqual(name1,name2);
}

test_shadow_name(100);

func test_array_map(){
    var list = ["hello"," ","world"];
    assertEqual(typeof(list),"array");
    assertEqual(len(list),3);
    assertEqual(list[0],"hello");
    list = append(list,"script");
    assertEqual(len(list),4);
    assertEqual(list[3],"script");
    list = append(list,100);
    assertEqual(len(list),5);
    assertEqual(typeof(list[4]),"integer");

    var sub = list[2:];
    assertEqual(sub[0],"world");
    assertEqual(len(sub),3);
    
    sub = list[2:4];
    assertEqual(len(sub),2);
    assertEqual(sub[0],"world");
    assertEqual(sub[1],"script");
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

    #dic key type must same type
    var dic = {"accept":"text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9",
     "accept-encoding":"gzip, deflate, br",
     "referer":"https://www.google.com/",
     "accpet-languare":"zh-CN,zh;q=0.9,en;q=0.8",
     "X":1000
    };

    #this expression will throw runtime exception
    #dic[800]= "800";

    assertEqual(typeof(dic),"map");
    assertEqual(len(dic),5);
    assertEqual(dic["X"],1000);
    dic["800"]= "800";
    assertEqual(dic["800"],"800");
    assertEqual(dic["accept-encoding"],"gzip, deflate, br");

    #that is ok,the key is same
    var dic2 ={100:"100",200:"200",300:"300",400:"400",3.14:3.1415926};
    assertEqual(dic2[100],"100");
    assertEqual(dic2[3.14],3.1415926);
}

test_array_map();

```
## 参考
https://github.com/stdpain/compiler-interpreter
