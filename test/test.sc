#for statement test 
#
#

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