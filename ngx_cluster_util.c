/*
 * =====================================================================================
 *
 *       Filename:  ngx_cluster_util.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2020年11月23日 17时04分33秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  etomc2@etomc2.com (), etomc2@etomc2.com
 *   Organization:  etomc2.com
 *
 * =====================================================================================
 */
/*
BSD 3-Clause License

Copyright (c) 2020, fastdbcache
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <libgen.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ngx_cluster.h"

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * nindex Description:
 * =====================================================================================
 */
size_t nindex(const char *s, int c) {
    size_t i, m = 0;
    for (i = 0; i < strlen(s); i++) {
        if (*(s + i) == c) {
            m = i;
        }
    }

    return m;
} /* -----  end of function nindex  ----- */
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * pindex Description:
 * =====================================================================================
 */
size_t pindex(const char *s, size_t len, int c) {
    size_t i, m = 0;
    for (i = 0; i < len; i++) {
        if (*(s + i) == c) {
            m = i;
        }
    }

    return m;
} /* -----  end of function pindex  ----- */
/*
 * ===  FUNCTION
 * ======================================================================
 *         Name:  get_uri
 *  Description:
 * =====================================================================================
 */
ngx_str_t get_uri(ngx_http_request_t *r) {
    ngx_str_t tmp_uri;

    if (r->uri.len >= (NGX_MAX_UINT32_VALUE / 4) - 1) {
        r->uri.len /= 4;
    }

    tmp_uri.len =
        r->uri.len +
        (2 * ngx_escape_uri(NULL, r->uri.data, r->uri.len, NGX_ESCAPE_ARGS));
    tmp_uri.data = ngx_pcalloc(r->pool, tmp_uri.len + 1);
    if (!tmp_uri.data) {
        tmp_uri.len = 0;
        return tmp_uri;
    }
    ngx_escape_uri(tmp_uri.data, r->uri.data, r->uri.len, NGX_ESCAPE_ARGS);

    return tmp_uri;
} /* -----  end of function get_uri  ----- */

/*
 * ===  FUNCTION
 * ======================================================================
 *         Name:  config_create_dir
 *  Description:
 * =====================================================================================
 */
int config_create_dir(const char *path, const mode_t mode) {
    struct stat st;
    char *pathnew = NULL;
    char *parent_dir_path = NULL;

    if (strcmp(path, "/") == 0)  // No need of checking if we are at root.
        return 0;

    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        // Check and create parent dir tree first.
        pathnew = strdup(path);
        if (pathnew == NULL) return -2;
        parent_dir_path = dirname(pathnew);
        if (config_create_dir(parent_dir_path, mode) == -1) {
            free(pathnew);

            return -1;
        }
        // Create this dir.
        if (mkdir(path, mode) == -1) {
            free(pathnew);

            return -1;
        }

        free(pathnew);
    }

    return 0;

} /* -----  end of function config_create_dir  ----- */

/*
 * ===  FUNCTION
 * ======================================================================
 *         Name:  config_create_file
 *  Description:
 * =====================================================================================
 */
void config_create_file(const char *path, const char *cont, int len) {
    FILE *fp;
    fp = fopen(path, "w+");
    if (fp == NULL) return;
    fprintf(fp, "%.*s", len, cont);
    fflush(fp);
    fclose(fp);
} /* -----  end of function config_create_file  ----- */
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * config_file_exist Description:
 * =====================================================================================
 */
int config_file_exist(const char *file) {
    struct stat st = {0};
    return stat(file, &st);
} /* -----  end of function config_file_exist  ----- */
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * config_file_read Description:
 * =====================================================================================
 */
int config_file_read(const char *file, ngx_str_t *data, ngx_http_request_t *r) {
    long sz;
    int fpi;
    sz = config_file_size(file);
    if (sz == 0) return -1;
    data->len = (int)sz;
    data->data = ngx_pcalloc(r->pool, data->len);
    fpi = open(file, O_RDONLY);
    int n = read(fpi, data->data, data->len);

    if (n == -1) return -1;

    return 0;

} /* -----  end of function config_file_read  ----- */

long config_file_size(const char *file) {
    FILE *fp;
    long sz;
    fp = fopen(file, "r");
    if (fp == NULL) return -1;

    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    fclose(fp);
    if (sz == 0) return -1;
    return sz;
}
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_dir_list_config Description:
 * =====================================================================================
 */
ngx_str_t *ngx_dir_list_config(ngx_http_request_t *r, ngx_str_t *cpath,
                               ngx_http_cluster_loc_conf_t *lccf) {
    ngx_int_t rc;
    ngx_err_t err;
    ngx_str_t new_path, *path;
    ngx_dir_t dir;
    u_char *config_file;
    int exist;
    const char *needle[] = {".conf", ".upstream", ".location"};
    char *pn;
    size_t needlen, m, fnext;
    ngx_str_t *name;
    size_t len;
    size_t cextlen;
    size_t plen;
    char *dp;
    char *pext;
    char *dirs = NULL;
    long fsize;
    ngx_str_t cfile, nfile, *contfile, szfile;
    /** u_char *ep; */
    ngx_str_t *dir_json = NULL, *dir_tmp;
    const char *fmtf = "{\"name\":\"%.*s\", \"size\":%d}";
    const char *fmtfs = "%.*s,{\"name\":\"%.*s\", \"size\":%d}";

    const char *fmt = "{\"name\":\"%s\",\"file\":[%.*s]}";
    const char *fmt_dir = "{\"name\":\"%s\",\"file\":[%.*s],\"dir\":[%.*s]}";

    ngx_str_t *resData, *subData = NULL;
    ngx_str_t resfile, file_tmp;

    resfile.len = 0;
    resfile.data = NULL;

    exist = config_file_exist((char *)cpath->data);
    if (exist == -1) {
        /** NX_DEBUG("exist:%d, cpath:%V", exist, cpath); */
        config_create_dir((char *)cpath->data, 0755);

        dp = index((char *)cpath->data, '.');
        len = dp - (char *)cpath->data;
        cextlen = pindex((char *)cpath->data, len - 1, '/');
        pext = index((char *)cpath->data + len, '/');
        /** NX_DEBUG("pext:%s",pext); */
        plen = strlen(pext);
        /** NX_DEBUG("len:%d, cextlen:%d,plen:%d",len,cextlen,plen); */

        path = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
        path->len = cextlen + plen + 1;
        /** path->len += 1; */
        path->data = ngx_pcalloc(r->pool, path->len);
        snprintf((char *)path->data, path->len, "%.*s%.*s", (int)cextlen,
                 cpath->data, (int)plen, pext);

    } else {
        path = cpath;
    }
    /** NX_DEBUG("path:%V", path); */
    if (ngx_open_dir(path, &dir) == NGX_ERROR) {
        err = ngx_errno;

        if (err == NGX_ENOENT || err == NGX_ENOTDIR ||
            err == NGX_ENAMETOOLONG) {
            rc = NGX_HTTP_NOT_FOUND;

        } else if (err == NGX_EACCES) {
            rc = NGX_HTTP_FORBIDDEN;

        } else {
            rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        NX_DEBUG("dir list config error::%d", rc);
        return NULL;
    }

    for (;;) {
        ngx_set_errno(0);

        if (ngx_read_dir(&dir) == NGX_ERROR) {
            err = ngx_errno;

            if (err != NGX_ENOMOREFILES) {
                NX_DEBUG(" \"%V\" failed", path);
                return NULL;
            }

            break;
        }
        config_file = ngx_de_name(&dir);

        len = ngx_de_namelen(&dir);

        if (config_file[0] == '.') {
            continue;
        }
        if (ngx_de_is_dir(&dir) != 1) {
            fnext = 0;
            if (lccf->ngx_cluster_filter != NULL) {
                name = lccf->ngx_cluster_filter->elts;

                for (m = 0; m < lccf->ngx_cluster_filter->nelts; m++) {
                    needlen = nindex((char *)config_file, '.');
                    pn = (char *)config_file + needlen;
                    if (ngx_strncmp(pn, name[m].data, name[m].len) == 0 &&
                        (len - needlen) == name[m].len) {
                        fnext = 1;
                        break;
                    }
                }
            } else {
                for (m = 0; m < 3; m++) {
                    needlen = nindex((char *)config_file, '.');
                    pn = (char *)config_file + needlen;
                    if (ngx_strncmp(pn, needle[m], strlen(needle[m])) == 0 &&
                        (len - needlen) == strlen(needle[m])) {
                        fnext = 1;
                        break;
                    }
                }
            }
            if (fnext == 0) continue;

            if (exist == -1) {
                cfile.len = cpath->len + len + 1;
                cfile.data = ngx_pcalloc(r->pool, cfile.len);
                snprintf((char *)cfile.data, cfile.len, "%s/%s", cpath->data,
                         config_file);

                nfile.len = path->len + len + 1;
                nfile.data = ngx_pcalloc(r->pool, nfile.len);
                snprintf((char *)nfile.data, nfile.len, "%s/%s", path->data,
                         config_file);

                contfile = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
                config_file_read((char *)nfile.data, contfile, r);
                config_create_file((char *)cfile.data, (char *)contfile->data,
                                   contfile->len);
            }

            szfile.len = snprintf(NULL, 0, "%.*s/%s", (int)path->len,
                                  path->data, config_file);
            szfile.len += 1;
            szfile.data = ngx_pcalloc(r->pool, szfile.len);
            snprintf((char *)szfile.data, szfile.len, "%.*s/%s", (int)path->len,
                     path->data, config_file);
            fsize = config_file_size((char *)szfile.data);

            if (resfile.len == 0) {
                resfile.len = snprintf(NULL, 0, fmtf, len, config_file, fsize);
                resfile.len += 1;
                resfile.data = ngx_pcalloc(r->pool, resfile.len);
                snprintf((char *)resfile.data, resfile.len, fmtf, len,
                         config_file, fsize);
            } else {
                file_tmp.len = snprintf(NULL, 0, fmtfs, resfile.len,
                                        resfile.data, len, config_file, fsize);
                file_tmp.len += 1;
                file_tmp.data = ngx_pcalloc(r->pool, file_tmp.len);
                snprintf((char *)file_tmp.data, file_tmp.len, fmtfs,
                         resfile.len, resfile.data, len, config_file, fsize);
                ngx_pfree(r->pool, resfile.data);

                resfile.len = file_tmp.len;
                resfile.data = file_tmp.data;
            }
        } else {
            new_path.len = cpath->len + len;
            new_path.len += 2;
            new_path.data = ngx_pcalloc(r->pool, new_path.len);
            if (new_path.data == NULL) {
                return NULL;
            }
            snprintf((char *)new_path.data, new_path.len, "%s/%s", cpath->data,
                     config_file);

            subData = ngx_dir_list_config(r, &new_path, lccf);
            if (dir_json == NULL) {
                dir_json = subData;
            } else {
                dir_tmp = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
                dir_tmp->len =
                    snprintf(NULL, 0, "%.*s,%.*s", (int)dir_json->len,
                             dir_json->data, (int)subData->len, subData->data);
                dir_tmp->len += 1;
                dir_tmp->data = ngx_pcalloc(r->pool, dir_tmp->len);
                snprintf((char *)dir_tmp->data, dir_tmp->len, "%.*s,%.*s",
                         (int)dir_json->len, dir_json->data, (int)subData->len,
                         subData->data);
                ngx_pfree(r->pool, dir_json->data);
                ngx_pfree(r->pool, dir_json);
                dir_json = dir_tmp;
            }
        }
    }

    if (ngx_close_dir(&dir) == NGX_ERROR) {
        NX_DEBUG(" \"%V\" failed", path);
    }
    dirs = rindex((char *)path->data, '/');

    resData = ngx_pcalloc(r->pool, sizeof(ngx_str_t));

    if (resData == NULL) return NULL;
    if (dir_json == NULL || dir_json->data == NULL) {
        resData->len =
            snprintf(NULL, 0, fmt, dirs + 1, resfile.len, resfile.data);
        resData->len += 1;
        resData->data = ngx_pcalloc(r->pool, resData->len);
        if (resData->data == NULL) return NULL;
        snprintf((char *)resData->data, resData->len, fmt, dirs + 1,
                 resfile.len, resfile.data);
    } else {
        resData->len = snprintf(NULL, 0, fmt_dir, dirs + 1, resfile.len,
                                resfile.data, dir_json->len, dir_json->data);
        resData->len += 1;
        resData->data = ngx_pcalloc(r->pool, resData->len);
        if (resData->data == NULL) return NULL;
        snprintf((char *)resData->data, resData->len, fmt_dir, dirs + 1,
                 resfile.len, resfile.data, dir_json->len, dir_json->data);
    }
    return resData;
} /* -----  end of function ngx_dir_list_config  ----- */
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_commit_config Description:
 * =====================================================================================
 */
void ngx_commit_config(ngx_http_request_t *r, ngx_str_t *path,
                       ngx_http_cluster_loc_conf_t *lccf) {
    ngx_int_t rc;
    ngx_err_t err;
    ngx_dir_t dir;
    u_char *config_file;
    size_t len, tlen;
    size_t cextlen;
    ngx_str_t new_path;
    ngx_str_t nfile, *cpath, rpath, *contfile;

    size_t plen;
    char *dp;
    char *pext;
    const char *needle[] = {".conf", ".upstream", ".location"};
    char *pn;
    size_t needlen, m, fnext;
    ngx_str_t *name;

    if (ngx_open_dir(path, &dir) == NGX_ERROR) {
        err = ngx_errno;

        if (err == NGX_ENOENT || err == NGX_ENOTDIR ||
            err == NGX_ENAMETOOLONG) {
            rc = NGX_HTTP_NOT_FOUND;

        } else if (err == NGX_EACCES) {
            rc = NGX_HTTP_FORBIDDEN;

        } else {
            rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        NX_DEBUG("dir list config error::%d", rc);
        return;
    }
    for (;;) {
        ngx_set_errno(0);

        if (ngx_read_dir(&dir) == NGX_ERROR) {
            err = ngx_errno;

            if (err != NGX_ENOMOREFILES) {
                NX_DEBUG(" \"%V\" failed", path);
                return;
            }

            break;
        }
        config_file = ngx_de_name(&dir);

        len = ngx_de_namelen(&dir);

        if (config_file[0] == '.') {
            continue;
        }
        if (ngx_de_is_dir(&dir) != 1) {
            fnext = 0;
            if (lccf->ngx_cluster_filter != NULL) {
                name = lccf->ngx_cluster_filter->elts;

                for (m = 0; m < lccf->ngx_cluster_filter->nelts; m++) {
                    needlen = nindex((char *)config_file, '.');
                    pn = (char *)config_file + needlen;
                    if (ngx_strncmp(pn, name[m].data, name[m].len) == 0 &&
                        (len - needlen) == name[m].len) {
                        fnext = 1;
                        break;
                    }
                }
            } else {
                for (m = 0; m < 3; m++) {
                    needlen = nindex((char *)config_file, '.');
                    pn = (char *)config_file + needlen;
                    if (ngx_strncmp(pn, needle[m], strlen(needle[m])) == 0 &&
                        (len - needlen) == strlen(needle[m])) {
                        fnext = 1;
                        break;
                    }
                }
            }
            if (fnext == 0) continue;

            dp = index((char *)path->data, '.');
            tlen = dp - (char *)path->data;
            cextlen = pindex((char *)path->data, tlen - 1, '/');
            pext = index((char *)path->data + tlen, '/');
            plen = strlen(pext);

            cpath = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
            cpath->len = cextlen + plen + len + 2;
            cpath->data = ngx_pcalloc(r->pool, cpath->len);
            snprintf((char *)cpath->data, cpath->len, "%.*s%.*s/%.*s",
                     (int)cextlen, path->data, (int)plen, pext, (int)len,
                     config_file);

            rpath.len = cextlen + plen + 1;
            rpath.data = ngx_pcalloc(r->pool, rpath.len);
            snprintf((char *)rpath.data, rpath.len, "%.*s%.*s", (int)cextlen,
                     path->data, (int)plen, pext);

            nfile.len = path->len + len + 1;
            nfile.data = ngx_pcalloc(r->pool, nfile.len);
            snprintf((char *)nfile.data, nfile.len, "%s/%s", path->data,
                     config_file);

            contfile = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
            config_file_read((char *)nfile.data, contfile, r);

            config_create_dir((char *)rpath.data, 0755);
            config_create_file((char *)cpath->data, (char *)contfile->data,
                               contfile->len);
        } else {
            new_path.len = path->len + len;
            new_path.len += 2;
            new_path.data = ngx_pcalloc(r->pool, new_path.len);
            if (new_path.data == NULL) {
                return;
            }
            snprintf((char *)new_path.data, new_path.len, "%s/%s", path->data,
                     config_file);
            ngx_commit_config(r, &new_path, lccf);
        }
    }

} /* -----  end of function ngx_commit_config  ----- */
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * post_body_data Description:
 * =====================================================================================
 */
void post_body_data(ngx_http_request_t *r, ngx_http_post_body_t *pb) {
    ngx_str_t *key;
    ngx_chain_t *cl, *hcl;
    u_char *p, *ampersand, *equal, *last;
    size_t len;
    ngx_http_post_body_t *hp = NULL;
    ngx_str_t *base_tmp = NULL;

    if (r->request_body == NULL) {
        NX_DEBUG("REQUEST_BODY null");
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    if (r->request_body->bufs == NULL) {
        NX_DEBUG("bufs null");
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    key = ngx_pcalloc(r->pool, sizeof(ngx_str_t));

    hcl = r->request_body->bufs;
    for (cl = hcl; cl; cl = cl->next) {
        p = cl->buf->pos;
        last = cl->buf->last;
        equal = NULL;
        ampersand = NULL;

        for (; p < last;) {
            len = 0;

            ampersand = ngx_strlchr(p, last, '&');
            if (ampersand == NULL || ampersand > last) {
                ampersand = last;
            }

            equal = ngx_strlchr(p, last, '=');

            if ((equal == NULL) || (equal > ampersand) ||
                ((equal + 1) == ampersand) || ((equal + 2) == ampersand)) {
                len = ampersand - p;
                /** NX_DEBUG("DATA_ext:%s", p); */
                if (hp != NULL) {
                    base_tmp = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
                    base_tmp->len =
                        snprintf(NULL, 0, "%.*s%.*s", (int)hp->data.len,
                                 hp->data.data, (int)len, p);
                    base_tmp->len += 1;
                    base_tmp->data = ngx_pcalloc(r->pool, base_tmp->len);
                    snprintf((char *)base_tmp->data, base_tmp->len, "%.*s%.*s",
                             (int)hp->data.len, hp->data.data, (int)len, p);
                    ngx_pfree(r->pool, hp->data.data);
                    hp->data.data = base_tmp->data;
                    hp->data.len = base_tmp->len;
                }
                equal = ampersand;
                p = ampersand + 1;
                continue;
            }

            key->data = p - len;
            key->len = equal - p - len;
            /** NX_DEBUG("key:%V", key); */

            hp = pb;
            post_body_get(hp, key->data);
            if (hp != NULL) {
                hp->data.len = ampersand - equal;
                hp->data.data = ngx_pcalloc(r->pool, hp->data.len);
                snprintf((char *)hp->data.data, hp->data.len, "%s", equal + 1);
            }

            p = ampersand + 1;
        }
    }
    return;
} /* -----  end of function post_body_data  ----- */
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * post_body_header_out Description:
 * =====================================================================================
 */
ngx_int_t post_body_header_out(ngx_http_request_t *r, ngx_str_t *resData,
                               int html_json, int finalize) {
    ngx_int_t rc;
    ngx_buf_t *b;
    ngx_chain_t out;

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    out.buf = b;
    out.next = NULL;
    b->pos = resData->data; /* first position in memory of the data */
    b->last = resData->data + resData->len -
              1;     /* last position in memory of the data */
    b->memory = 1;   /* content is in read-only memory */
    b->last_buf = 1; /* there will be no more buffers in the request */

    if (html_json == 0) {
        r->headers_out.content_type.len = sizeof("text/html") - 1;
        r->headers_out.content_type.data = (u_char *)"text/html";
    } else {
        r->headers_out.content_type.len = sizeof("application/json") - 1;
        r->headers_out.content_type.data = (u_char *)"application/json";
    }

    /* Sending the headers for the reply. */
    r->headers_out.status = NGX_HTTP_OK; /* 200 status code */
    /* Get the content length of the body. */
    r->headers_out.content_length_n = resData->len - 1;
    ngx_http_send_header(r); /* Send the headers */
    rc = ngx_http_output_filter(r, &out);
    if (finalize == 1) ngx_http_finalize_request(r, rc);
    return rc;
} /* -----  end of function post_body_header_out  ----- */

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * build_base_dir Description:
 * =====================================================================================
 */
void build_base_dir(ngx_http_request_t *r, ngx_str_t *base_dir,
                    ngx_http_cluster_loc_conf_t *lccf) {
    const char *dir_ext = CONFIG_EXT_PATH;

    size_t tlen = nindex((char *)lccf->confname, '/');
    size_t len = pindex((char *)lccf->confname, tlen - 1, '/');
    size_t elen = tlen - len - 1;

    base_dir->len = tlen + 1 + strlen(dir_ext) + elen;
    base_dir->data = ngx_pcalloc(r->pool, base_dir->len);

    snprintf((char *)base_dir->data, base_dir->len, "%.*s%s%.*s", (int)tlen,
             lccf->confname, dir_ext, (int)elen, (lccf->confname + len + 1));

} /* -----  end of function build_base_dir  ----- */

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * new_server_config_file Description:
 * =====================================================================================
 */
void new_server_config_file(ngx_http_request_t *r, const char *type,
                            ngx_str_t *cfile, ngx_str_t server,
                            ngx_str_t domain) {
    cfile->len = snprintf(NULL, 0, "%.*s%.*s%s", (int)server.len, server.data,
                          (int)domain.len, domain.data, type);
    cfile->len += 1;
    cfile->data = ngx_pcalloc(r->pool, cfile->len);
    snprintf((char *)cfile->data, cfile->len, "%.*s%.*s%s.conf",
             (int)server.len, server.data, (int)domain.len, domain.data, type);

} /* -----  end of function new_server_config_file  ----- */
