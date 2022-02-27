/*
see RFC 3986: 
Appendix A.  Collected ABNF for URI

  URI           = scheme ":" hier-part [ "?" query ] [ "" fragment ]

   hier-part     = "//" authority path-abempty
                 / path-absolute
                 / path-rootless
                 / path-empty

   URI-reference = URI / relative-ref

   absolute-URI  = scheme ":" hier-part [ "?" query ]

   relative-ref  = relative-part [ "?" query ] [ "" fragment ]

   relative-part = "//" authority path-abempty
                 / path-absolute
                 / path-noscheme
                 / path-empty

   scheme        = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )

   authority     = [ userinfo "@" ] host [ ":" port ]
   userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )
   host          = IP-literal / IPv4address / reg-name
   port          = *DIGIT

   IP-literal    = "[" ( IPv6address / IPvFuture  ) "]"

   IPvFuture     = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )

   IPv6address   =                            6( h16 ":" ) ls32
                 /                       "::" 5( h16 ":" ) ls32
                 / [               h16 ] "::" 4( h16 ":" ) ls32
                 / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
                 / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
                 / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
                 / [ *4( h16 ":" ) h16 ] "::"              ls32
                 / [ *5( h16 ":" ) h16 ] "::"              h16
                 / [ *6( h16 ":" ) h16 ] "::"

   h16           = 1*4HEXDIG
   ls32          = ( h16 ":" h16 ) / IPv4address
   IPv4address   = dec-octet "." dec-octet "." dec-octet "." dec-octet */


#include <string>
#include <sstream>
#include <iostream>

#include "uri.h"

namespace kit_server
{

/* machine */

%%{
    machine uri_parser;

    sub_delims    = ( "!" | "$" | "$" | "&" | "'" | "(" | ")" | "*"
                    | "+" | "," | ";" | "=") ;
    gen_delims    = ( ":" | "/" | "?" | "#" | "[" | "]" | "@" ) ;
    reserved      = ( gen_delims | sub_delims ) ;
    unreserved    = ( alpha | digit | "-" | "." | "_" | "~" ) ;
    pct_encoded   = ( "%" xdigit xdigit ) ;

    action marku { mark = fpc; }
    action markh { mark = fpc; }

    action save_scheme
    {
        uri->setScheme(std::string(mark, fpc - mark));
        mark = NULL;
    }

    scheme = (alpha (alpha | digit | "+" | "-" | ".")*) >marku %save_scheme;


    action save_port
    {
        if(fpc != mark){
            uri->setPort(atoi(mark));
        }
        mark = NULL;
    }

    action save_userinfo
    {
        if(mark) {
            uri->setUserinfo(std::string(mark, fpc - mark));
        }
        mark = NULL;
    }

    action save_host
    {
        if(mark != NULL) {
            uri->setHost(std::string(mark, fpc - mark));
        }
    }

    userinfo = (unreserved | pct_encoded | sub_delims | ":")*;
    dec_octet = digit | [1-9] digit | "1" digit{2} | 2 [0-4] digit | "25" [0-5];
    IPv4address = dec_octet "." dec_octet "." dec_octet "." dec_octet;
    h16 = xdigit{1,4};
    ls32 = (h16 ":" h16) | IPv4address;
    IPv6address = (                            ( h16 ":" ) {6} ls32) |
                  (                       "::" ( h16 ":" ) {5} ls32) |
                  ((               h16 )? "::" ( h16 ":" ) {4} ls32) |
                  ((( h16 ":" ){1} h16 )? "::" ( h16 ":" ) {3} ls32) |
                  ((( h16 ":" ){2} h16 )? "::" ( h16 ":" ) {2} ls32) |
                  ((( h16 ":" ){3} h16 )? "::" ( h16 ":" ) {1} ls32) |
                  ((( h16 ":" ){4} h16 )? "::"                 ls32) |
                  ((( h16 ":" ){5} h16 )? "::"                 h16 ) |
                  ((( h16 ":" ){6} h16 )? "::"                     );
    IPvFuture = "v" xdigit+ "." (unreserved | sub_delims | ":")+;
    IP_literal = "[" (IPv6address | IPvFuture) "]";
    reg_name = (unreserved | pct_encoded | sub_delims)*;
    host = IP_literal | IPv4address | reg_name;
    port = digit*;

    authority = ( (userinfo %save_userinfo "@")? host >markh %save_host (":" port >markh %save_port)? ) >markh;

    action save_segment
    {
        mark = NULL;
    }

    action save_path
    {
        uri->setPath(std::string(mark, fpc - mark));
        mark = NULL;
    }

#    pchar = unreserved | pct_encoded | sub_delims | ":" | "@";
# add (any -- ascii) support chinese

    # 支持中文
    pchar = ( (any -- ascii)  | unreserved | pct_encoded | sub_delims | ":" | "@" );
    segment = pchar*;
    segment_nz = pchar+;
    segment_nz_nc = (pchar - ":")+;

    action clear_segments
    {  
    }

    path_abempty = (("/" segment))? ("/" segment)*;
    path_absolute = ("/" (segment_nz ("/" segment)*)?);
    path_noscheme = segment_nz_nc ("/" segment)*;
    path_rootless = segment_nz ("/" segment)*;
    path_empty = "";
    path = (path_abempty | path_absolute | path_noscheme | path_rootless | path_empty);


    action save_query
    {
        uri->setQuery(std::string(mark, fpc - mark));
        mark = NULL;
    }

    action save_fragment
    {
        uri->setFragment(std::string(mark, fpc - mark));
        mark = NULL;
    }

    query = (pchar | "/" | "?")* >marku %save_query;
    fragment = (pchar | "/" | "?")* >marku %save_fragment;

    hier_part = ("//" authority path_abempty > markh %save_path) | path_absolute | path_rootless | path_empty;

    relative_part = ("//" authority path_abempty) | path_absolute | path_noscheme | path_empty;
    relative_ref = relative_part ("?" query)? ("#" fragment)?;

    absolute_URI = scheme ":" hier_part ("?" query)?;

    # Obsolete, but referenced from HTTP, so we translate
    relative_URI = relative_part ("?" query)?;

    URI = scheme ":" hier_part ("?" query)? ("#" fragment)?;
    URI_reference = URI | relative_ref;
    # 状态机入口
    main := URI_reference;

    write data;
}%%

/* exec */

Uri::Uri()
    :m_port(0)
{

}


int16_t Uri::getPort() const
{
    if(m_port)
        return m_port;
    
    if(m_scheme == "http")
        return 80;
    else if(m_scheme == "https")
        return 443;

    return 0;
}

std::ostream& Uri::dump(std::ostream& os) const
{
    os << m_scheme << "://"
        << m_userinfo
        << (m_userinfo.size() ? "@" : "")
        << m_host
        << (isDefaultPort() ? "" : ":" + std::string("" + m_port))
        << (m_path.size() ? m_path : "/")
        << (m_query.size() ? "?" : "")
        << m_query
        << (m_fragment.size() ? "#" : "")
        << m_fragment;

    return os;  
}

std::string Uri::toString() const
{
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

//通过URI创建一个地址 
Address::ptr Uri::createAddress() const
{
    auto addr = Address::LookUpAnyIPAddress(m_host);
    if(addr)
        addr->setPort(getPort());

    return addr;
}

//传入一个uri字符串解析出对应URI对象
Uri::ptr Uri::Create(const std::string& u)
{
    Uri::ptr uri(new Uri);

    /*cs mark状态机中用到的变量 提前定义*/
    int cs = 0;
    const char* mark = NULL;

    //初始化自动状态机
    %% write init;
    //指向u头指针
    const char *p = u.c_str();
    //指向u尾指针
    const char *pe = p + u.size();
    //ragel指定的一个尾指针的名字
    const char *eof = pe;
    //调用自动状态机
    %% write exec;

    //uri_parser_error 检查解析是否错误
    if(cs == uri_parser_error)
        return nullptr;
    else if(cs >= uri_parser_first_final) //解析完成后的状态码是否合法  
        return uri;

    return nullptr;

}


bool Uri::isDefaultPort() const
{
    if(m_port == 0)
        return true;

    if(m_scheme == "http")
        return m_port == 80;
    else if(m_scheme == "https")
        return m_port == 443;

    return false;
}



}