
$(function () {
   
    
    var diff_main = {}
    var diff_node = {}
    var edit_path = "";
    //var domain_info;
    var domain_main;
    var domain_node;
    var ng_url = "http://localhost:1234";

    upload_config = function () {

        var conf_path = $("#config_path").text();

        if (conf_path.substr(0, 4) == "Main") {
            o = "/main";
        } else {
            o = "/node";
        }

        var data = cm.getValue();
        var base64_data = Base64.encode(data);
        // console.log(base64_data);
        var formData = "base64=" + base64_data + "&path=" + edit_path;  //Name value Pair

        var settings = {
            "url": ng_url + o + "/upload/config",
            type: "POST",
            data: formData,
            "Content-Type": "application/x-www-form-urlencoded",

        };
        // var val = $("#config_path").html();
        // var nval = "<span class='ui-icon ui-icon-alert' style='float: left; margin-right: .3em;'></span>"+val;
        // $("#config_path").html(nval);
        $.ajax(settings).complete(function (response) {
            // $("#config_path").html(val);
            //cm.setValue(Base64.decode(response.responseText));
            alert("ok")
        }).error(function (e) {

        });

    }

    delete_server = function (p) {


        if (p.substr(0, 4) == "Main") {
            o = "/main";
        } else {
            o = "/node";
        }

        var next = p.indexOf("/");
        var pp = p.substr(next+1);
        next = pp.indexOf("/");
        var path = pp.substr(next);
        

        var settings = {
            "url": ng_url + o + "/delete/server",
            type: "POST",
            data: "domain=" + path,
            "Content-Type": "application/x-www-form-urlencoded",
        };

        $.ajax(settings).complete(function (response) {
            console.log(response.responseText);
            init_tree("/main");
            init_tree("/node");
            alert("删除完成");
        }).error(function (e) {

        });
    }

    commit_config = function () {
        var conf_path = $("#config_path").text();

        if (conf_path.substr(0, 4) == "Main") {
            o = "/main";
        } else {
            o = "/node";
        }

        $.ajax({
            url: ng_url + o + "/commit/config",
            type: "POST",
            "Content-Type": "application/x-www-form-urlencoded",
            complete: function (e) {
                if (e.status == 200) {

                    var data = e.responseText;

                    // var jdata = JSON.parse(data);

                    console.log(data);
                    alert("ok");
                }
            }
        });
    }



    listDomain = function (root, jdata,o) {
        var name = jdata['name'] + "/";

        root += name;

        var files = jdata['file'];

        for (let index = 0; index < files.length; index++) {
            const file = files[index];
            var fname = file['name'];
            var fsize = file['size'];
            if(o=="/main"){
                diff_main[root + fname] = fsize;
            }else{
                diff_node[root + fname] = fsize;
            }
            
        }

        if (jdata.hasOwnProperty('dir')) {
            var dirs = jdata['dir'];
            for (let index = 0; index < dirs.length; index++) {
                const dir = dirs[index];
                listDomain(root, dir,o);
            }
        }

    }

    diffDomain = function (jcurrent, japply) {
        // -1 delete
        // 0 not change
        // 1 change
        // 2 new file
        var jcat = {}
        jcat = jcurrent;
        for (var key in jcat) {
            if (japply.hasOwnProperty(key)) {
                if (japply[key] != jcat[key]) {
                    jcat[key] = 1
                } else {
                    jcat[key] = 0
                }
            } else {
                jcat[key] = 2
            }
        }

        for (var key in japply) {
            if (!jcat.hasOwnProperty(key)) {
                jcat[key] = -1;
            }
        }
        return jcat;
    }

    baseName = function (str) {
        var base = new String(str).substring(str.lastIndexOf('/') + 1);
        // if (base.lastIndexOf(".") != -1)
        // 	base = base.substring(0, base.lastIndexOf("."));
        return base;
    }

    //对比
    get_list = function (o) {
        $.ajax({
            url: ng_url + o + "/get/list?r=1",
            type: "POST",
            "Content-Type": "application/x-www-form-urlencoded",
            complete: function (e) {
                
                if (e.status == 200) {
                    var apply_file = {}
                    var current_file = {}
                    var data = e.responseText;

                    var jdata = JSON.parse(data);
                    
                    
                    if(o=="/main"){
                        listDomain("", domain_main,o);
                        current_file = diff_main;
                        diff_main = {}
                        listDomain("", jdata,o);
                        apply_file = diff_main;
                    }else{
                        listDomain("", domain_node,o);
                        current_file = diff_node;
                        diff_node = {}
                        listDomain("", jdata,o);
                        apply_file = diff_node;
                    }
                   
                    console.log(current_file)
                    console.log(apply_file)
                    var jcat = diffDomain(current_file, apply_file);
                    var html = "";
                    for (var key in jcat) {
                        var icon = "";
                        if (jcat[key] == 0) {
                            icon = "ui-icon ui-icon-check";
                        } else if (jcat[key] == 1) {
                            icon = "ui-icon ui-icon-wrench";
                        } else if (jcat[key] == 2) {
                            icon = "ui-icon ui-icon-plus";
                        } else {
                            icon = "ui-icon ui-icon-trash";
                        }
                        html += "<li><a href='javascript:void(0);' onclick='push_file(\"" + key + "\")' title='" + key + "'><span class='" + icon + "' style='float: left; margin-right: .3em;'></span>" + baseName(key) + "</a></li>";
                    }
                    if(o=="/main"){
                        $("#main_list").html(html);
                    }else{
                        $("#node_list").html(html);
                    }
                    
                }
            }
        });
    }

    init_treeview = function () {
        //treeview for inner menus
        $("#browser").treeview({
            toggle: function () {
                var par_path = getPath(this);
                if (par_path.length == 0) {
                    par_path = $(this).parents().find(">span").filter(":first").text();
                }
                var path = $(this).find(">span").text();
                var del = "";
                
                if (par_path.substr(-6) == "vhosts") {
                    del = '<a href="javascript:void(0)"  onclick="delete_server(\'' + par_path + "/" + path + '\')"><span'
                        + ' class="ui-icon ui-icon-trash"'
                        + ' style="float: left; margin-right: .3em;"></span>'
                        + '<strong>' + path + '</strong></a>';
                    cm.setValue("");
                } else {

                    del = par_path + " &raquo; <span class='here'>" + path + "</span>";
                }
               // $("#ngx_edit").attr("class","hide");
                $("#config_path").html(del)
            }
        });



        $(".file").click(function () {
            var conf_path = getPath(this);
            // console.log(conf_path.substr(0, 4));
            $("#ngx_edit").attr("class","");
            if (conf_path.substr(0, 4) == "Main") {
                get_config(conf_path, "/main");
            } else {
                get_config(conf_path, "/node");
            }


        });
    }

    init_tree = function (o) {
        $.ajax({
            url: ng_url + o + "/get/list",
            type: "POST",
            "Content-Type": "application/x-www-form-urlencoded",
            complete: function (e) {
                if (e.status == 200) {

                    var data = e.responseText;

                    var jdata = JSON.parse(data);
                    if(o=="/main"){
                        domain_main = jdata;
                    }else{
                        domain_node = jdata;
                    }
                    
                    if (!jdata.hasOwnProperty("status")) {
                        var tree_html = dir_json(jdata);
                        if (o == "/main") {
                            $("#main_config").html(tree_html);
                        } else {
                            $("#node_config").html(tree_html);
                        }


                    }

                    init_treeview();

                }
            }
        });
    }

    get_version = function (o) {
        
        $.ajax({
            url: ng_url + o + "/version",
            type: "POST",
            "Content-Type": "application/x-www-form-urlencoded",
            complete: function (e) {
                if (e.status == 200) {
                    var data = e.responseText;

                    var jdata = JSON.parse(data);
                    console.log(jdata)
                    $("#version").html(jdata['version']);

                }
            }
        });
    }

    get_config = function (conf_path, o) {
        $("#config_path").html("");
        $("#config_path").html(conf_path)
        //把内容写到 textarea 上面去的	

        var start = conf_path.indexOf('/');
        conf_path = conf_path.substr(start + 1);
        start = conf_path.indexOf('/');
        conf_path = conf_path.substr(start + 1);
        edit_path = conf_path;
        var d = cm.getValue();
        var b64 = Base64.encode(d);

        var settings = {
            "url": ng_url + o + "/get/config",
            type: "POST",
            data: "path=" + conf_path,
            "Content-Type": "application/x-www-form-urlencoded",
        };

        $.ajax(settings).complete(function (response) {
            var source = response.responseText;
            cm.setValue(source);
        }).error(function (e) {

        });
    }

    getPath = function (a) {
        var path = "";
        var $parent = $(a).parents("li").find(">span");

        if ($parent.length > 1) {

            const element = $parent.filter(
                function (i, e) {

                    path = $(this).text() + "/" + path;
                    return path;
                }
            );
        }

        return path.substr(0, path.length - 1);
    }

    dir_json = function (data) {
        var files = data['file'];
        var name = data['name'];
        var dir_html = "";
        if (data.hasOwnProperty('dir')) {

            for (let index = 0; index < data['dir'].length; index++) {
                const element = data['dir'][index];
                dir_html += dir_json(element);
            }
        }
        var subfile_li = "";
        for (let index = 0; index < files.length; index++) {
            var cfile = files[index]['name'];
            subfile_li += '<li><span class="file"><a href="#">' + cfile + '</a></span></li>';
        }

        var file_li = '<li class="closed"><span class="folder">' + name + '</span>'
            + '<ul>'
            + subfile_li + dir_html
            + '</ul>'
            + '</li>';
        return file_li;
    }

    new_server = function () {
        $('#dialog').dialog('open');
    }

    add_new_servre = function (domain) {
        var conf_path = $("#config_path").text();

        if (conf_path.substr(0, 4) == "Main") {
            o = "/main";
        } else {
            o = "/node";
        }
        $.ajax({
            url: ng_url + o + "/new/server",
            type: "POST",
            data: "domain=" + domain,
            "Content-Type": "application/x-www-form-urlencoded",
            complete: function (e) {
                if (e.status == 200) {

                    var data = e.responseText;
                    // console.log(data)
                    //var jdata = JSON.parse(data);
                    $('#dialog').dialog("close");
                    init_tree("/main");
                    init_tree("/node");
                }
            }
        });
    }

    push_file = function (key) {

        var html = "提交当前文件:" + key + "<br/> <span class='ui-icon ui-icon-alert' style='float: left; margin-right: .3em;'></span>"
            + "<strong>注意！</strong>该文件，将从编辑状态转为应用状态!";
        console.log(html)
        $('#dialg_push_text').html(html);
        $('#dialog_push').dialog('open');
    }

    format_code = function () {
        var source = cm.getValue();
        source = js_beautify(source, {
            "indent_size": 4,
            "space_in_paren": true,
            "space_in_empty_paren": true,
            "space_after_anon_function": true,
            "space_after_named_function": true,
            "brace_style": "collapse",
            "unindent_chained_methods": true,
            "break_chained_methods": true,
            "keep_array_indentation": true,
            "indent_char": " "
        }
        );
        regex = /\/ /gi;
        source = source.replaceAll(regex, "/");
        regex = /: /gi;
        source = source.replaceAll(regex, ":");
        //console.log(source)
        cm.setValue(source);
    }

});


