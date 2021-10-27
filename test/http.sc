
#https://github.com/Clivebi/onescript
func parse_url(url){
    var part = url,host_port;
    var result ={"scheme":"http"};
    var index = IndexString(part,":");
    if(index == -1){
        return nil;
    }
    result["scheme"] = part[:index];
    part = part[index:];
    if(!HasPrefixString(part,"://")){
        return nil;
    }
    part = part[3:];
    index = IndexString(part,"/");
    if(index == -1){
        host_port = part;
    }else{
        host_port = part[:index]; 
    }
    if(ContainsString(host_port,":")){
        var temp = SplitString(host_port,":");
        result["host"] = temp[0];
        result["port"] = ToInteger(temp[1]);
    }else{
        result["host"] = host_port;
    }
    if(index == -1){
        result["path"] = "/";
        return result;
    }
    part = part[index:];
    if(len(part) == 0){
        part = "/";
    }
    index = IndexString(part,"?");
    if(index == -1){
        result["path"] = part;
    }else{
        result["path"] = part[:index];
    }
    return result;
}

#Println(parse_url("https://github.com/"));
#Println(parse_url("https://github.com"));
#Println(parse_url("https://github.com/Clivebi/onescript"));
#Println(parse_url("https://github.com:80/Clivebi/onescript?key=12dkjdjdj"));


func build_http_request_header(method,path,host,header_field){
    var req = "",k,v;
    req = method+" "+path+" "+"HTTP/1.1\r\n";
    header_field["Host"] = host;
    for k,v in header_field{
        req += k;
        req += ": ";
        req += v;
        req +="\r\n";
    }
    return req;  
}


var header ={"Connection":"keep-alive"};
header["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9";
header["Accept-Encoding"] = "gzip, deflate, br";
header["Accept-Language"] = "zh-CN,zh;q=0.9,en;q=0.8";
header["User-Agent"]="Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/94.0.4606.81 Safari/537.36";

var uri = parse_url("https://www.baidu.com");
var req = build_http_request_header("GET",uri["path"],uri["host"],header);
Println(req);