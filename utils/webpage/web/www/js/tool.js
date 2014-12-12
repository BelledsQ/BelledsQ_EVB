function $id(id) {
    return document.getElementById(id);
}
var is =
{
    types: ["Array", "Boolean", "Date", "Number", "Object", "RegExp", "String", "Window", "HTMLDocument"]
}
for (var i = 0, c; c = is.types[i++]; ) {
    is[c] = (function (type) {
        return function (obj) {
            return Object.prototype.toString.call(obj) == "[object " + type + "]";
        }
    })(c);
}
var api = {
    apiurl: "/cgi-bin/SysInfo",
    airplay:"<getSysInfo><airplay/></getSysInfo>",
    ssid: "<getSysInfo><SSID/></getSysInfo>",
    version: "<getSysInfo><Version/></getSysInfo>",
    storage: "<getSysInfo><Storage/></getSysInfo>",
    power: "<getSysInfo><Power/></getSysInfo>",
    network: "<getSysInfo><WorkMode/><Client/></getSysInfo>",
    joinwired: "<getSysInfo><JoinWired/></getSysInfo>",
    aplist: "<getSysInfo><APList/><RemoteAP/></getSysInfo>",
    currentap: "<getSysInfo><RemoteAP/></getSysInfo>",
    setdhcp: "<setSysInfo><JoinWired><DHCP/></JoinWired></setSysInfo>",
    samba: "<getSysInfo><SAMBA/></getSysInfo>",
    dms: "<getSysInfo><DMS/></getSysInfo>",
    ftp: "<getSysInfo><FTP/></getSysInfo>",
    webdav: "<getSysInfo><WebDAV/></getSysInfo>",
    advance: "<getSysInfo><SAMBA/><DMS/><FTP/><WebDAV/></getSysInfo>"
};
//去除字符串首尾空格
String.prototype.trim = function () {
    var reg = /^\s*(.*?)\s*$/gim;
    return this.replace(reg, "$1");
};
String.prototype.Length = function () {
    var blen = 0;
    for (var i = 0, l = this.length; i < l; i++) {
        if ((this.charCodeAt(i) & 0xff00) != 0) {
            blen++;
        }
        blen++;
    }
    return blen;
}
//
if (!String._FORMAT_SEPARATOR) {
    String._FORMAT_SEPARATOR = String.fromCharCode(0x1f);
    String._FORMAT_ARGS_PATTERN = new RegExp('^[^' + String._FORMAT_SEPARATOR + ']*'
            + new Array(100).join('(?:.([^' + String._FORMAT_SEPARATOR + ']*))?'));
}
if (!String.format)
    String.format = function (s) {
        return Array.prototype.join.call(arguments, String._FORMAT_SEPARATOR).
            replace(String._FORMAT_ARGS_PATTERN, s);
    }
if (!''.format)
    String.prototype.format = function () {
        return (String._FORMAT_SEPARATOR +
            Array.prototype.join.call(arguments, String._FORMAT_SEPARATOR)).
            replace(String._FORMAT_ARGS_PATTERN, this);
    }
   /*
　 *　方法:Array.remove(dx)
　 *　功能:删除数组元素.
　 *　参数:dx删除元素的下标.
　 *　返回:在原数组上修改数组
　 */
    //经常用的是通过遍历,重构数组.
    Array.prototype.remove = function (dx) {
        if (isNaN(dx) || dx > this.length) { return false; }
        for (var i = 0, n = 0; i < this.length; i++) {
            if (this[i] != this[dx]) {
                this[n++] = this[i]
            }
        }
        this.length -= 1
    }
    Array.prototype.container=function(key){
        for (var i = 0, n = 0; i < this.length; i++) {
            if (this[i]==key) return true;
        }
        return false;
    }
    function Hashtable() {
        this._hash = new Object();
        this.add = function (key, value) {
            if (typeof (key) != "undefined") {
                if (this.contains(key) == false) {
                    this._hash[key] = typeof (value) == "undefined" ? null : value;
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
        this.remove = function (key) { delete this._hash[key]; }
        this.count = function () { var i = 0; for (var k in this._hash) { i++; } return i; }
        this.get = function (key) { return this._hash[key]; }
        this.contains = function (key) { return typeof (this._hash[key]) != "undefined"; }
        this.clear = function () { for (var k in this._hash) { delete this._hash[k]; } }

    }
function DoWait(interval, callback) {
    if (interval < 0) {
        $("#divLoading").find("div:last").html("");
        callback && callback();
    }
    else {
        $("#divLoading").find("div:last").html(interval + "s");
        setTimeout(function () { DoWait(interval, callback); }, 1000);
        interval--;
    }
}
function checkIP(ip) {
    if (ip == undefined || ip.trim() == "") return false;
    //ip地址
    var exp = /^(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$/;
    if (ip.match(exp) == null) return false;
    return true;
}
function paddingUrl(url) {
    return url.indexOf("?") == -1 ? url + "?" : url;
}
function Request(name, defaultvalue) {
    var b = decodeURIComponent(unescape(window.location.href), true);
    var a = new RegExp("" + name + "=([^&]*)", "ig");
    return ((b.match(a)) ? (b.match(a)[b.match(a).length - 1].substr(name.length + 1)) : defaultvalue)
}
function appendTS(url) {
    if (url.indexOf('ts=') == -1) {
        url = url + (url.indexOf('?') == -1 ? "?ts=" : "&ts=") + (new Date()).valueOf().toString();
    } else {
        url = url.replace(/&ts=.+/, "&ts=" + (new Date()).valueOf().toString())
    }
    return url;
}
function pageHeight() {
    if ($.browser.msie) {
        return document.compatMode == "CSS1Compat" ? document.documentElement.clientHeight : document.body.clientHeight;
    } else {
        return self.innerHeight;
    }
};
//返回当前页面宽度 
function pageWidth() {
    if ($.browser.msie) {
        return document.compatMode == "CSS1Compat" ? document.documentElement.clientWidth : document.body.clientWidth;
    } else {
        return self.innerWidth;
    }
    //return $("body").width();
};
function Loading(msg, interval, callback) {
    if ($id("divLoading") == null) {
        //
        $("<div class=\"boxlayer\" style=\"position:absolute;left:0px;top:0px;width:" + pageWidth() + "px;height:" + pageHeight() + "px;filter:alpha(opacity=30); -moz-opacity:0.3; -khtml-opacity: 0.3; opacity: 0.3;background-color:#33393C;z-index:9999;\"></div>" +
          "<div id=\"divLoading\" style=\"position:absolute;height:80px;background:#E6EFFA;border:solid 4px #82B2EE;padding:8px;z-index:100000;\">" +
            "<div style=\"float:left;background:url(/css/images/loading.gif);width:0px;height:0px;\"></div>" +
            "<div style=\"float:left;padding:8px;\" class=\"txt div_sp_text smfont\">正在加载数据...</div>" +
            "<div style=\"color:red;padding:8px 0px;font-family:微软雅黑;\"></div>"+
          "</div>").appendTo($("body"));
    }
    if (msg == "") {
        $("#divLoading").hide();
        $("div.boxlayer").show();
        return;
    }
    $("#divLoading")
        .find("div:eq(1)").html(msg).end()
        .find("div:last").html("   ");
    var wd = 60;
    $("#divLoading").children("div").show(); //.each(function () { wd += $(this).width(); });.width(wd)
    if (interval && interval > 0) $("#divLoading").find("div:last").html(interval + "s");
    $("#divLoading")
        .css("left", ((pageWidth()-$("#divLoading").width()-24) / 2) + "px")
        .css("top", (pageHeight() / 2 - (parseInt($("#divLoading").height()) / 2)) + "px")
        .show();
    $("div.boxlayer").show();
    //
    interval && interval > 0 && DoWait(interval, callback);
}
function Loaded() {
    $("#divLoading").hide();
    $("div.boxlayer").hide();
    interval = 0;
}

function HtmlEncode(s) {
    if (s==undefined || !is.String(s)) return s.toString();
    s = s.replace(/&/g, "&amp;");
    s = s.replace(/</g, "&lt;");
    s = s.replace(/>/g, "&gt;");
    return s.replace(/\"/g, "&quot;");
}
function HtmlDecode(s) {
    if (s == undefined || s.length == 0) return "";
    s = s.replace(/&amp;/g, "&");
    s = s.replace(/&lt;/g, "<");
    s = s.replace(/&gt;/g, ">");
    s = s.replace(/&nbsp;/g, " ");
    s = s.replace(/&#39;/g, "\'");
    s = s.replace(/&quot;/g, "\"");
    return s;
}
function toAttribute(obj) {
    for (var k in obj) {
        if (k.charAt(0) != "@") {
            obj["@" + k] = obj[k];
            delete obj[k];
        }
    }
    return obj;
}
function parseXml(xml) {
    var dom = null;
    if (window.DOMParser) {
        try {
            dom = (new DOMParser()).parseFromString(xml, "text/xml");
        }
        catch (e) { dom = null; }
    }
    else if (window.ActiveXObject) {
        try {
            dom = new ActiveXObject('Microsoft.XMLDOM');
            dom.async = false;
            if (!dom.loadXML(xml)) // parse error ..
                window.alert(dom.parseError.reason + dom.parseError.srcText);
        }
        catch (e) { dom = null; }
    }
    else
        alert("oops");
    return dom;
}
var X = {
    toObj: function (xml) {
        var o = {};
        if (xml.nodeType == 1) {   // element node ..
            if (xml.attributes.length)   // element with attributes  ..
                for (var i = 0; i < xml.attributes.length; i++)
                    o["" + xml.attributes[i].nodeName] = (xml.attributes[i].nodeValue || "").toString();
            if (xml.firstChild) { // element has child nodes ..
                var textChild = 0, cdataChild = 0, hasElementChild = false;
                for (var n = xml.firstChild; n; n = n.nextSibling) {
                    if (n.nodeType == 1) hasElementChild = true;
                    else if (n.nodeType == 3 && n.nodeValue.match(/[^ \f\n\r\t\v]/)) textChild++; // non-whitespace text
                    else if (n.nodeType == 4) cdataChild++; // cdata section node
                }
                if (hasElementChild) {
                    if (textChild < 2 && cdataChild < 2) { // structured element with evtl. a single text or/and cdata node ..
                        X.removeWhite(xml);
                        for (var n = xml.firstChild; n; n = n.nextSibling) {
                            if (n.nodeType == 3)  // text node
                                o["#text"] = X.escape(n.nodeValue);
                            else if (n.nodeType == 4)  // cdata node
                                o["#cdata"] = X.escape(n.nodeValue);
                            else if (o[n.nodeName]) {  // multiple occurence of element ..
                                if (o[n.nodeName] instanceof Array)
                                    o[n.nodeName][o[n.nodeName].length] = X.toObj(n);
                                else
                                    o[n.nodeName] = [o[n.nodeName], X.toObj(n)];
                            }
                            else  // first occurence of element..
                                o[n.nodeName] = X.toObj(n);
                        }
                    }
                    else { // mixed content
                        if (!xml.attributes.length)
                            o = X.escape(X.innerXml(xml));
                        else
                            o["#text"] = X.escape(X.innerXml(xml));
                    }
                }
                else if (textChild) { // pure text
                    if (!xml.attributes.length)
                        o = X.escape(X.innerXml(xml));
                    else
                        o["#text"] = X.escape(X.innerXml(xml));
                }
                else if (cdataChild) { // cdata
                    if (cdataChild > 1)
                        o = X.escape(X.innerXml(xml));
                    else
                        for (var n = xml.firstChild; n; n = n.nextSibling)
                            o["#cdata"] = X.escape(n.nodeValue);
                }
            }
            if (!xml.attributes.length && !xml.firstChild) o = null;
        }
        else if (xml.nodeType == 9) { // document.node
            o = X.toObj(xml.documentElement);
        }
        else
            alert("unhandled node type: " + xml.nodeType);
        return o;
    },
    toJson: function (o, name, ind) {
        var json = name ? ("\"" + name + "\"") : "";
        if (o instanceof Array) {
            for (var i = 0, n = o.length; i < n; i++)
                o[i] = X.toJson(o[i], "", ind + "\t");
            json += (name ? ":[" : "[") + (o.length > 1 ? ("\n" + ind + "\t" + o.join(",\n" + ind + "\t") + "\n" + ind) : o.join("")) + "]";
        }
        else if (o == null)
            json += (name && ":") + "null";
        else if (typeof (o) == "object") {
            var arr = [];
            for (var m in o)
                arr[arr.length] = X.toJson(o[m], m, ind + "\t");
            json += (name ? ":{" : "{") + (arr.length > 1 ? ("\n" + ind + "\t" + arr.join(",\n" + ind + "\t") + "\n" + ind) : arr.join("")) + "}";
        }
        else if (typeof (o) == "string")
            json += (name && ":") + "\"" + o.toString() + "\"";
        else
            json += (name && ":") + o.toString();
        return json;
    },
    innerXml: function (node) {
        var s = ""
        if ("innerHTML" in node)
            s = node.innerHTML;
        else {
            var asXml = function (n) {
                var s = "";
                if (n.nodeType == 1) {
                    s += "<" + n.nodeName;
                    for (var i = 0; i < n.attributes.length; i++)
                        s += " " + n.attributes[i].nodeName + "=\"" + (n.attributes[i].nodeValue || "").toString() + "\"";
                    if (n.firstChild) {
                        s += ">";
                        for (var c = n.firstChild; c; c = c.nextSibling)
                            s += asXml(c);
                        s += "</" + n.nodeName + ">";
                    }
                    else
                        s += "/>";
                }
                else if (n.nodeType == 3)
                    s += n.nodeValue;
                else if (n.nodeType == 4)
                    s += "<![CDATA[" + n.nodeValue + "]]>";
                return s;
            };
            for (var c = node.firstChild; c; c = c.nextSibling)
                s += asXml(c);
        }
        return s;
    },
    escape: function (txt) {
        return txt.replace(/[\\]/g, "\\\\")
                   .replace(/[\"]/g, '\\"')
                   .replace(/[\n]/g, '\\n')
                   .replace(/[\r]/g, '\\r');
    },
    removeWhite: function (e) {
        e.normalize && e.normalize();
        for (var n = e.firstChild; n; ) {
            if (n.nodeType == 3) {  // text node
                if (!n.nodeValue.match(/[^ \f\n\r\t\v]/)) { // pure whitespace text node
                    var nxt = n.nextSibling;
                    e.removeChild(n);
                    n = nxt;
                }
                else
                    n = n.nextSibling;
            }
            else if (n.nodeType == 1) {  // element node
                X.removeWhite(n);
                n = n.nextSibling;
            }
            else                      // any other node
                n = n.nextSibling;
        }
        return e;
    }
};
function xml2json(xml, tab) {

    if (xml.nodeType == 9) // document node
        xml = xml.documentElement;
    var json = X.toJson(X.toObj(X.removeWhite(xml)), xml.nodeName, "\t");
    return "{\n" + tab + (tab ? json.replace(/\t/g, tab) : json.replace(/\t|\n/g, "")) + "\n}";
}
function xml2object(xml) {
    if (is.String(xml)) xml = parseXml(xml);
    return X.toObj(X.removeWhite(xml));
}
function json2xml(o, tab) {
   var toXml = function(v, name, ind) {
      var xml = "";
      if (v instanceof Array) {
         for (var i=0, n=v.length; i<n; i++)
            xml += ind + toXml(v[i], name, ind+"\t") + "\n";
      }
      else if (typeof(v) == "object") {
         var hasChild = false;
         xml += ind + "<" + name;
         for (var m in v) {
            if (m.charAt(0) == "@")
               xml += " " + m.substr(1) + "=\"" + v[m].toString() + "\"";
            else
               hasChild = true;
         }
         xml += hasChild ? ">" : "/>";
         if (hasChild) {
            for (var m in v) {
               if (m == "#text")
                  xml += v[m];
               else if (m == "#cdata")
                  xml += "<![CDATA[" + v[m] + "]]>";
               else if (m.charAt(0) != "@")
                  xml += toXml(v[m], m, ind+"\t");
            }
            xml += (xml.charAt(xml.length-1)=="\n"?ind:"") + "</" + name + ">";
         }
      }
      else {
         xml += ind + "<" + name + ">" + v.toString() +  "</" + name + ">";
      }
      return xml;
   }, xml="";
   for (var m in o)
      xml += toXml(o[m], m, "");
   return tab ? xml.replace(/\t/g, tab) : xml.replace(/\t|\n/g, "");
}