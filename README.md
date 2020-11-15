
<!-- PROJECT SHIELDS -->
<!--
*** I'm using markdown "reference style" links for readability.
*** Reference links are enclosed in brackets [ ] instead of parentheses ( ).
*** See the bottom of this document for the declaration of the reference variables
*** for contributors-url, forks-url, etc. This is an optional, concise syntax you may use.
*** https://www.markdownguide.org/basic-syntax/#reference-style-links
-->
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![MIT License][license-shield]][license-url]

<!-- PROJECT LOGO -->
<br />
<p align="center">
  <a href="https://github.com/vyouzhi">
    <img src="https://avatars2.githubusercontent.com/u/5832145?s=400&u=e1923037c2831a3de8e1bb5b3305c1434b85981d&v=4" alt="Logo" width="80" height="80">
  </a>

  <h3 align="center">ngx_http_cluster</h3>

  <p align="center">
    ngx_http_cluster is nginx module cluster main Control any nodes!
    <br />
    ngx_http_cluster 是 nginx 集群管理配置 的模块插件
    <br />
    <a href="https://github.com/vyouzhis/ngx_http_cluster"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="https://github.com/vyouzhis/ngx_http_cluster">View Demo</a>
    ·
    <a href="https://github.com/vyouzhis/ngx_http_cluster/issues">Report Bug</a>
    ·
    <a href="https://github.com/vyouzhis/ngx_http_cluster/issues">Request Feature</a>
  </p>
</p>



<!-- TABLE OF CONTENTS -->
## 栏目(Table of Contents)

* [关于该项目(About the Project)](#关于该项目about-the-project)
  * [依赖关系(Built With)](#依赖关系built-with)
* [起始(Getting Started)](#起始getting-started)
  * [编译安装(Installation)](#编译安装installation)
* [配置(Usage)](#配置usage)
* [集群(Cluster)](#集群Cluster)
* [WebAPI](#WebAPI)
* [License](#license)
* [联系(Contact)](#联系contact)



<!-- ABOUT THE PROJECT -->
## 关于该项目(About the Project)

采用 ngx_http_subrequest API 方式来管理不同节点的配置文件：

**拥有热更新参数(Runtime Configuration)**

### 依赖关系(Built With)
需要用到的软件版本.
* [nginx](http://nginx.org/en/download.html)
* [ngx_http_cluster](https://github.com/vyouzhis/ngx_http_cluster)



<!-- GETTING STARTED -->
## 起始(Getting Started)

需要下载以下的文件.

### 编译安装(Installation)

1. 在这儿选择 nginx 的版本 [http://nginx.org/download/nginx-1.18.0.tar.gz](http://nginx.org/en/download.html)
2. 下载nginx
```sh
wget http://nginx.org/download/nginx-1.18.0.tar.gz
```
3. 解压
```sh
tar -zxvf nginx-1.18.0.tar.gz
```
4. git clone ngx_http_cluster
```JS
git clone https://github.com/vyouzhis/ngx_http_cluster.git
```
5. 编译安装
```sh
./configure --add-module=../ngx_http_cluster --with-http_ssl_module
gmake
gmake install
```


<!-- USAGE EXAMPLES -->
## 配置(Usage)

> cluster action

| 指令        | 区域           | 说明  |
| ------------- |:-------------:| -----:|
| ngx_cluster_main      | http | 是否启用该模块:on or off |


## 集群(Cluster)

>main cluster

| 指令        | 区域           | 说明  |
| ------------- |:-------------:| -----:|
|cluster_api | location | web api  |
| ngx_cluster_branch      | location | cluster baranch,setting in main server |

> node cluster

| 指令        | 区域           | 说明  |
| ------------- |:-------------:| -----:|
|ngx_cluster_node | location | node Control  |


>use nginx ngx_http_subrequest for cluster

```
                        user web api Control nodes

                       +-------------------------+
                       |    main nginx server    |
                       +------------+------------+
                                    |
                +-------------------+-----------------+
                |                   |                 |
                |                   |                 |
   +------------+-----+   +---------+---------+    +--+---------------+
   |node1 nginx server|   | node2 nginx server|    |node3 nginx server|
   +------------------+   +-------------------+    +------------------+


```

> 参考配置 [nginx example conf](https://github.com/vyouzhis/ngx_http_cluster/tree/master/doc/main_nginx.conf)


## WebAPI
|     restful api   | curl test           | 说明  |
| ------------- |:-------------:| -----:|
| update_conf | POST      |    在线更新配置,ngxconf:base64 config, path:nginx config file path  |

>update_conf
```
curl --location --request GET 'http://127.0.0.1:1234/upload_conf' \
--header 'ngxconf: Agc2VydmVyIHsKICAgICAgICBsaXN0ZW4gICAgICAgODAwMDsKICAgICMgICAgbGlzdGVuICAgICAgIHNvbWVuYW1lOjgwODA7CiAgICAgICAgc2VydmVyX25hbWUgIHd3dy5iYWlkdS5jb207CgogICAgICAgIGxvY2F0aW9uIC8gewogICAgICAgICAgICAgZmFzdGNnaV9wYXNzICAgMTI3LjAuMC4xOjkwMDA7CiAgICAgICAgICAgIGZhc3RjZ2lfaW5kZXggIGluZGV4LnBocDsKICAgICAgICAgICAgZmFzdGNnaV9wYXJhbSAgU0NSSVBUX0ZJTEVOQU1FICAvdXNyL2xvY2FsL25naW54L2h0bWwkZmFzdGNnaV9zY3JpcHRfbmFtZTsKICAgICAgICAgICAgaW5jbHVkZSAgICAgICAgZmFzdGNnaV9wYXJhbXM7CiAgICAgICAgfQogICAgfQo=' \
--header 'path: ./vhost/www.163.com.conf'

```

<!-- LICENSE -->
## License

Distributed under the MIT License. See `LICENSE` for more information.



<!-- CONTACT -->
## Contact

vyouzhi - [@github](https://github.com/vyouzhis/ngx_http_cluster) - vouzhi@gmail.com

<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/vyouzhis/ngx_http_cluster.svg?style=flat-square
[contributors-url]: https://github.com/vyouzhis/ngx_http_cluster/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/vyouzhis/ngx_http_cluster.svg?style=flat-square
[forks-url]: https://github.com/vyouzhis/ngx_http_cluster/network/members
[stars-shield]: https://img.shields.io/github/stars/vyouzhis/ngx_http_cluster.svg?style=flat-square
[stars-url]: https://github.com/vyouzhis/ngx_http_cluster/stargazers
[issues-shield]: https://img.shields.io/github/issues/vyouzhis/ngx_http_cluster.svg?style=flat-square
[issues-url]: https://github.com/vyouzhis/ngx_http_cluster/issues
[license-shield]: https://img.shields.io/github/license/vyouzhis/ngx_http_cluster.svg?style=flat-square
[license-url]: https://github.com/vyouzhis/ngx_http_cluster/blob/master/LICENSE.txt
[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=flat-square&logo=linkedin&colorB=555
[product-screenshot]: images/screenshot.png
