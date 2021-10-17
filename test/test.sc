#
var pass = true;
func assertEqual(a,b){
    if(a != b){
        Println("error here a=",a,"b=",b);
        pass = false;
    }
}

Println("测试算术运算与优先级");
assertEqual(1*(2+3),5);
assertEqual(1+1-1,1);
assertEqual(1+2*3,7);

Println("测试作用域");
var name1 = 0;
for (var name1 = 0; name1 < 6;name1++ ){
    Println(name1);
}
assertEqual(name1,0);

Println("测试break");
var name2 = 0;
for (var name1 = 0; name1 < 10;name1++ ){
    if(name2 > 5){
        break;
    }
    name2++;
}
assertEqual(name2,6);

Println("测试break2");
name2 = 1;
for {
    name2++;
    if(name2 > 5){
        break;
    }
}

assertEqual(name2,6);

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

if(pass){
    Println("所有测试通过");
}else{
    Println("测试不通过");
}