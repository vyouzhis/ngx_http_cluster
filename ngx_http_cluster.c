/*
 * =====================================================================================
 *
 *       Filename:  ngx_http_cluster.c
 *
 *    Description:  cluster
 *
 *        Version:  1.0
 *        Created:  2020年08月10日 11时40分20秒
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

#include "ngx_cluster.h"
#define WEB_NGX_CLUSTER " "

//--- define ---
/* The empty string. */
static u_char ngx_web_cluster[] = WEB_NGX_CLUSTER;
/**
 *   function
 */
static ngx_int_t ngx_http_cluster_dummy_init(ngx_conf_t *cf);
static void *ngx_http_cluster_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_cluster_merge_loc_conf(ngx_conf_t *cf, void *parent,
                                             void *child);

/*
 ** Module's registred function/handlers.
 */
static ngx_int_t ngx_http_cluster_access_handler(ngx_http_request_t *r);

/**
 * echo  empty;
 */
static char *ngx_http_cluster_web(ngx_conf_t *cf, ngx_command_t *cmd,
                                  void *conf);
static ngx_int_t ngx_http_cluster_web_handler(ngx_http_request_t *r);

/**
 * main web api
 */
static char *ngx_http_cluster_main(ngx_conf_t *cf, ngx_command_t *cmd,
                                   void *conf);
static ngx_int_t ngx_http_cluster_main_handler(ngx_http_request_t *r);
/**
 * node web api
 */
static char *ngx_http_cluster_node(ngx_conf_t *cf, ngx_command_t *cmd,
                                   void *conf);
static ngx_int_t ngx_http_cluster_node_handler(ngx_http_request_t *r);

/**
 *  branch function
 */

static char *ngx_http_branch(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_filter(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static void ngx_http_cluster_body_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_cluster_handler_internal(ngx_http_request_t *r);

/**
 * web route api;
 */

static void ngx_cluster_list_config(ngx_http_request_t *r);
static void ngx_cluster_get_config(ngx_http_request_t *r);
static void ngx_cluster_upload_config(ngx_http_request_t *r);
static void ngx_cluster_commit_config(ngx_http_request_t *r);
static void ngx_cluster_new_server(ngx_http_request_t *r);
static void ngx_cluster_delete_server(ngx_http_request_t *r);

static ngx_int_t ngx_http_cluster_rest(ngx_http_request_t *r);

/**
 * This module provided directive: cluster.
 *
 */
static ngx_command_t ngx_http_cluster_commands[] = {
    {ngx_string(NGX_CLUSTER_BRANCH),
     NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF |
         NGX_CONF_TAKE1,
     ngx_http_branch, NGX_HTTP_LOC_CONF_OFFSET, 0, NULL},

    {ngx_string(NGX_CLUSTER_SYNC),        /* directive */
     NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS, /* location context and takes
                                             no arguments*/
     ngx_http_cluster_web,                /* configuration setup function */
     0, /* No offset. Only one context is supported. */
     0, /* No offset when storing the module configuration on struct. */
     NULL},

    {ngx_string(NGX_CLUSTER_MAIN),        /* directive */
     NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS, /* location context and takes
                                             no arguments*/
     ngx_http_cluster_main,               /* configuration setup function */
     0, /* No offset. Only one context is supported. */
     0, /* No offset when storing the module configuration on struct. */
     NULL},

    {ngx_string(NGX_CLUSTER_NODE), NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
     ngx_http_cluster_node, 0, 0, NULL},

    {ngx_string(NGX_CLUSTER_CONF_PATH), NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
     ngx_conf_set_str_slot, NGX_HTTP_LOC_CONF_OFFSET,
     offsetof(ngx_http_cluster_loc_conf_t, ngx_cluster_conf_path), NULL},

    {ngx_string(NGX_CLUSTER_FILTER),
     NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF |
         NGX_CONF_TAKE1,
     ngx_http_filter, NGX_HTTP_LOC_CONF_OFFSET, 0, NULL},

    ngx_null_command /* command termination */
};

/* The module context. */
static ngx_http_module_t ngx_http_cluster_module_ctx = {
    NULL,                        /* preconfiguration */
    ngx_http_cluster_dummy_init, /* postconfiguration */
    NULL,                        /* create main configuration */
    NULL,                        /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    ngx_http_cluster_create_loc_conf, /* create location configuration */
    ngx_http_cluster_merge_loc_conf   /* merge location configuration */
};

/* Module definition. */
ngx_module_t ngx_http_cluster_module = {
    NGX_MODULE_V1,
    &ngx_http_cluster_module_ctx, /* module context */
    ngx_http_cluster_commands,    /* module directives */
    NGX_HTTP_MODULE,              /* module type */
    NULL,                         /* init master */
    NULL,                         /* init module */
    NULL,                         /* init process */
    NULL,                         /* init thread */
    NULL,                         /* exit thread */
    NULL,                         /* exit process */
    NULL,                         /* exit master */
    NGX_MODULE_V1_PADDING};
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_http_cluster_dummy_init Description:
 * =====================================================================================
 */
static ngx_int_t ngx_http_cluster_dummy_init(ngx_conf_t *cf) {
    ngx_http_handler_pt *h;
    ngx_http_core_main_conf_t *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    if (cmcf == NULL) return (NGX_ERROR); /*LCOV_EXCL_LINE*/

    /* Register for rewrite phase */
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_PRECONTENT_PHASE].handlers);
    if (h == NULL) return (NGX_ERROR); /*LCOV_EXCL_LINE*/

    *h = ngx_http_cluster_access_handler;

    return (NGX_OK);
} /* -----  end of function ngx_http_cluster_dummy_init  ----- */

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_http_cluster_create_loc_conf Description:
 * =====================================================================================
 */
static void *ngx_http_cluster_create_loc_conf(ngx_conf_t *cf) {
    ngx_http_cluster_loc_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_cluster_loc_conf_t));
    if (conf == NULL) return NULL;
    /** conf->ngx_cluster_main = NGX_CONF_UNSET_UINT; */
    /** conf->ngx_cluster_sync = NGX_CONF_UNSET_UINT; */
    conf->ngx_cluster_node = NGX_CONF_UNSET_UINT;

    conf->ngx_cluster_branch = NGX_CONF_UNSET_PTR;
    conf->confname = NGX_CONF_UNSET_PTR;
    conf->ngx_cluster_filter=NGX_CONF_UNSET_PTR;
    return (conf);

} /* -----  end of function ngx_http_cluster_create_loc_conf  ----- */

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_http_cluster_merge_loc_conf Description:
 * =====================================================================================
 */
static char *ngx_http_cluster_merge_loc_conf(ngx_conf_t *cf, void *parent,
                                             void *child) {
    ngx_http_cluster_loc_conf_t *prev = parent;
    ngx_http_cluster_loc_conf_t *conf = child;

    /** ngx_conf_merge_value(conf->ngx_cluster_main, prev->ngx_cluster_main, 0);
     */
    /** ngx_conf_merge_value(conf->ngx_cluster_sync, prev->ngx_cluster_sync, 0);
     */
    ngx_conf_merge_value(conf->ngx_cluster_node, prev->ngx_cluster_node, 0);
    ngx_conf_merge_ptr_value(conf->ngx_cluster_branch, prev->ngx_cluster_branch,
                             NULL);
    ngx_conf_merge_ptr_value(conf->ngx_cluster_filter, prev->ngx_cluster_filter,
                             NULL);
    ngx_conf_merge_ptr_value(conf->confname, prev->confname,
                             cf->conf_file->file.name.data);
    ngx_conf_merge_str_value(conf->ngx_cluster_conf_path,
                             prev->ngx_cluster_conf_path, "");

    return NGX_CONF_OK;
} /* -----  end of function ngx_http_cluster_merge_loc_conf  ----- */

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_http_cluster_access_handler Description:
 * =====================================================================================
 */
static ngx_int_t ngx_http_cluster_access_handler(ngx_http_request_t *r) {
    ngx_http_cluster_loc_conf_t *lccf;
    ngx_http_cluster_ctx_t *ctx;
    ngx_int_t rc;

    lccf = ngx_http_get_module_loc_conf(r, ngx_http_cluster_module);
    if (!lccf) {
        NX_LOG("ngx_http_top_header_filter error");
        return NGX_DECLINED;
    }

    if (lccf->ngx_cluster_branch == NULL) {
        return NGX_DECLINED;
    }
    if (r != r->main) {
        return NGX_DECLINED;
    }

    ctx = ngx_http_get_module_ctx(r, ngx_http_cluster_module);

    if (ctx) {
        return ctx->status;
    }

    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_cluster_ctx_t));
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    ctx->status = NGX_DONE;

    ngx_http_set_ctx(r, ctx, ngx_http_cluster_module);

    rc = ngx_http_read_client_request_body(r, ngx_http_cluster_body_handler);
    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        return rc;
    }
    ngx_http_finalize_request(r, NGX_DONE);
    return NGX_DONE;

} /* -----  end of function ngx_http_cluster_access_handler  ----- */
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_http_cluster_body_handler Description:
 * =====================================================================================
 */
static void ngx_http_cluster_body_handler(ngx_http_request_t *r) {
    ngx_http_cluster_ctx_t *ctx;

    ctx = ngx_http_get_module_ctx(r, ngx_http_cluster_module);

    ctx->status = ngx_http_cluster_handler_internal(r);

    r->preserve_body = 1;
    ngx_http_clear_content_length(r);
    ngx_http_clear_accept_ranges(r);
    ngx_http_weak_etag(r);

    r->write_event_handler = ngx_http_core_run_phases;
    ngx_http_core_run_phases(r);

} /* -----  end of function ngx_http_cluster_body_handler  ----- */
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_http_cluster_handler_internal Description:
 * =====================================================================================
 */
static ngx_int_t ngx_http_cluster_handler_internal(ngx_http_request_t *r) {
    ngx_str_t *name;
    ngx_uint_t i;
    ngx_http_request_t *sr;
    ngx_http_cluster_loc_conf_t *mlcf;

    mlcf = ngx_http_get_module_loc_conf(r, ngx_http_cluster_module);

    name = mlcf->ngx_cluster_branch->elts;
    /** ngx_str_t uri = get_uri(r); */

    /** NX_DEBUG("handler internal uri:%V", &uri); */
    for (i = 0; i < mlcf->ngx_cluster_branch->nelts; i++) {
        if (ngx_http_subrequest(r, &name[i], &r->args, &sr, NULL, 0) !=
            NGX_OK) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        sr->method = r->method;
        sr->method_name = r->method_name;
        sr->header_in = sr->header_in;
    }

    return NGX_DECLINED;

} /* -----  end of function ngx_http_cluster_handler_internal  ----- */

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_http_branch Description:
 * =====================================================================================
 */
static char *ngx_http_branch(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_cluster_loc_conf_t *mlcf = conf;
    ngx_str_t *value, *s;

    value = cf->args->elts;

    if (ngx_strcmp(value[1].data, "off") == 0) {
        if (mlcf->ngx_cluster_branch != NGX_CONF_UNSET_PTR) {
            return "is duplicate";
        }

        mlcf->ngx_cluster_branch = NULL;
        return NGX_CONF_OK;
    }

    if (mlcf->ngx_cluster_branch == NULL) {
        return "is duplicate";
    }

    if (mlcf->ngx_cluster_branch == NGX_CONF_UNSET_PTR) {
        mlcf->ngx_cluster_branch =
            ngx_array_create(cf->pool, 4, sizeof(ngx_str_t));
        if (mlcf->ngx_cluster_branch == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    s = ngx_array_push(mlcf->ngx_cluster_branch);
    if (s == NULL) {
        return NGX_CONF_ERROR;
    }

    *s = value[1];

    return NGX_CONF_OK;
} /* -----  end of function ngx_http_branch  ----- */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ngx_http_filter
 *  Description:  
 * =====================================================================================
 */
static char *ngx_http_filter(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_cluster_loc_conf_t *mlcf = conf;
    ngx_str_t *value, *s;

    value = cf->args->elts;

    if (ngx_strcmp(value[1].data, "off") == 0) {
        if (mlcf->ngx_cluster_filter != NGX_CONF_UNSET_PTR) {
            return "is duplicate";
        }

        mlcf->ngx_cluster_filter = NULL;
        return NGX_CONF_OK;
    }

    if (mlcf->ngx_cluster_filter == NULL) {
        return "is duplicate";
    }

    if (mlcf->ngx_cluster_filter == NGX_CONF_UNSET_PTR) {
        mlcf->ngx_cluster_filter =
            ngx_array_create(cf->pool, 4, sizeof(ngx_str_t));
        if (mlcf->ngx_cluster_filter == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    s = ngx_array_push(mlcf->ngx_cluster_filter);
    if (s == NULL) {
        return NGX_CONF_ERROR;
    }

    *s = value[1];

    return NGX_CONF_OK;
}		/* -----  end of function ngx_http_filter  ----- */
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_http_cluster_web Description:
 * =====================================================================================
 */
static char *ngx_http_cluster_web(ngx_conf_t *cf, ngx_command_t *cmd,
                                  void *conf) {
    ngx_http_core_loc_conf_t *clcf; /* pointer to core location configuration */

    /* Install the cluster handler. */
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_cluster_web_handler;

    return NGX_CONF_OK;
} /* -----  end of function ngx_http_cluster_web  ----- */

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_http_cluster_web_handler Description:
 * =====================================================================================
 */
static ngx_int_t ngx_http_cluster_web_handler(ngx_http_request_t *r) {
    ngx_buf_t *b;
    ngx_chain_t out;

    /* Set the Content-Type header. */
    r->headers_out.content_type.len = sizeof("text/plain") - 1;
    r->headers_out.content_type.data = (u_char *)"text/plain";

    /* Allocate a new buffer for sending out the reply. */
    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

    /* Insertion in the buffer chain. */
    out.buf = b;
    out.next = NULL; /* just one buffer */

    b->pos = ngx_web_cluster; /* first position in memory of the data */
    b->last = ngx_web_cluster + sizeof(ngx_web_cluster) -
              1;     /* last position in memory of the data */
    b->memory = 1;   /* content is in read-only memory */
    b->last_buf = 1; /* there will be no more buffers in the request */

    /* Sending the headers for the reply. */
    r->headers_out.status = NGX_HTTP_OK; /* 200 status code */
    r->chunked = 1;
    /* Get the content length of the body. */
    /** r->headers_out.content_length_n = sizeof(ngx_web_cluster) - 1; */
    ngx_http_send_header(r); /* Send the headers */

    /* Send the body, and return the status code of the output filter chain. */
    return ngx_http_output_filter(r, &out);

} /* -----  end of function ngx_http_cluster_web_handler  ----- */
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_http_cluster_main   Description:
 * =====================================================================================
 */
static char *ngx_http_cluster_main(ngx_conf_t *cf, ngx_command_t *cmd,
                                   void *conf) {
    ngx_http_core_loc_conf_t *clcf; /* pointer to core location configuration */

    /* Install the cluster node handler. */
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_cluster_main_handler;

    return NGX_CONF_OK;

} /* -----  end of function ngx_http_cluster_main  ----- */

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_http_cluster_main_handler Description:
 * =====================================================================================
 */
static ngx_int_t ngx_http_cluster_main_handler(ngx_http_request_t *r) {
    ngx_str_t *resData;
    ngx_int_t rc;
    ngx_str_t uri = get_uri(r);
    ngx_int_t start = 0;
    const char *version = "/version";
    u_char *p;
    if (!(r->method & (NGX_HTTP_POST))) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    rc = ngx_http_cluster_rest(r);

    if (rc == NGX_DONE) {
        return rc;
    }

    resData = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
    start = uri.len - strlen(version);
    p = uri.data + start;
    if (ngx_strncmp(p, version, strlen(version)) == 0) {
        resData->len = snprintf(NULL, 0, "{\"status\":0,\"version\":\"%s\"}",
                                NGX_CLUSTER_VERSION);
        resData->len += 1;
        resData->data = ngx_pcalloc(r->pool, resData->len);
        snprintf((char *)resData->data, resData->len,
                 "{\"status\":0,\"version\":\"%s\"}\"", NGX_CLUSTER_VERSION);
    } else {
        ngx_str_set(resData, "{\"status\":-1}\0");
    }
    rc = post_body_header_out(r, resData, 1, 0);

    /* Send the body, and return the status code of the output filter chain. */
    return rc;
} /* -----  end of function ngx_http_cluster_main_handler  ----- */

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_http_cluster_node Description:
 * =====================================================================================
 */
static char *ngx_http_cluster_node(ngx_conf_t *cf, ngx_command_t *cmd,
                                   void *conf) {
    ngx_http_core_loc_conf_t *clcf; /* pointer to core location configuration */

    /* Install the cluster node handler. */
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_cluster_node_handler;

    return NGX_CONF_OK;
} /* -----  end of function ngx_http_cluster_node  ----- */
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_http_cluster_node_handler Description:
 * =====================================================================================
 */
static ngx_int_t ngx_http_cluster_node_handler(ngx_http_request_t *r) {
    ngx_str_t *resData;
    ngx_int_t rc;
    rc = ngx_http_cluster_rest(r);

    if (rc == NGX_DONE) {
        return rc;
    }
    resData = ngx_pcalloc(r->pool, sizeof(ngx_str_t));

    ngx_str_set(resData, "{\"status\":-1}\0");
    rc = post_body_header_out(r, resData, 1, 0);

    /* Send the body, and return the status code of the output filter chain. */
    return rc;
} /* -----  end of function ngx_http_cluster_node_handler  ----- */

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_cluster_list_config   Description:
 * =====================================================================================
 */
static void ngx_cluster_list_config(ngx_http_request_t *r) {
    ngx_int_t rc;
    ngx_http_cluster_loc_conf_t *lccf;

    lccf = ngx_http_get_module_loc_conf(r, ngx_http_cluster_module);
    ngx_str_t *resData;
    size_t plen;
    rc = ngx_http_discard_request_body(r);

    if (rc != NGX_OK) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    ngx_str_t *base_dir;
    base_dir = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
    if (r->args.len == 0) {
        build_base_dir(r, base_dir, lccf);
    } else {
        plen = nindex((char *)lccf->confname, '/');
        base_dir->len = plen + 1;
        base_dir->data = ngx_pcalloc(r->pool, base_dir->len);
        snprintf((char *)base_dir->data, base_dir->len, "%s", lccf->confname);
    }
    /** NX_DEBUG("base_dir:%V", base_dir); */

    resData = ngx_dir_list_config(r, base_dir,lccf);
    post_body_header_out(r, resData, 1, 1);
} /* -----  end of function ngx_cluster_list_config  ----- */
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_cluster_commit_config Description:
 * =====================================================================================
 */
static void ngx_cluster_commit_config(ngx_http_request_t *r) {
    ngx_str_t *path, *res;
    ngx_http_cluster_loc_conf_t *lccf;

    lccf = ngx_http_get_module_loc_conf(r, ngx_http_cluster_module);
    if (!lccf) {
        NX_LOG("ngx_http_top_header_filter error");
        return;
    }
    path = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
    build_base_dir(r, path, lccf);

    ngx_commit_config(r, path,lccf);
    res = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
    ngx_str_set(res, "{\"status\":0}\0");
    post_body_header_out(r, res, 1, 1);
} /* -----  end of function ngx_cluster_commit_config  ----- */
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_cluster_new_server Description:
 * =====================================================================================
 */
static void ngx_cluster_new_server(ngx_http_request_t *r) {
    ngx_str_t *res;
    ngx_str_t domain, new_server, *cfile, def_cont,ups_cont,loc_cont;
    const char *vhost = CONFIG_VHOSTS_PATH;
    const char *cont_fmt = CONFIG_DEFAULT_SERVER;
    const char *ups_fmt = CONFIG_DEFAULT_UPSTREAM;
    const char *loc_fmt = CONFIG_DEFAULT_LOCATION;
    ngx_str_t *base_dir;
    ngx_http_post_body_t *pb, *hp;

    ngx_http_cluster_loc_conf_t *lccf;

    lccf = ngx_http_get_module_loc_conf(r, ngx_http_cluster_module);
    if (!lccf) {
        NX_LOG("ngx_http_top_header_filter error");
        return;
    }

    pb = ngx_pcalloc(r->pool, sizeof(ngx_http_post_body_t));
    hp = pb;
    post_body_add(r, hp, "domain");
    post_body_data(r, pb);
    hp = pb;
    post_body_get(hp, "domain");

    if (hp->data.len == 0) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    domain = hp->data;

    base_dir = ngx_pcalloc(r->pool, sizeof(ngx_str_t));

    build_base_dir(r, base_dir, lccf);

    new_server.len =
        snprintf(NULL, 0, "%.*s%s%.*s/", (int)base_dir->len, base_dir->data,
                 vhost, (int)domain.len, domain.data);
    new_server.len += 2;
    new_server.data = ngx_pcalloc(r->pool, new_server.len);
    snprintf((char *)new_server.data, new_server.len, "%.*s%s%.*s/",
             (int)base_dir->len, base_dir->data, vhost, (int)domain.len,
             domain.data);
    /** NX_DEBUG("new_server:%V", &new_server); */

    config_create_dir((char *)new_server.data, 0755);

    cfile = ngx_pcalloc(r->pool, sizeof(ngx_str_t));

    const char *mtype = ".conf";
    new_server_config_file(r, mtype, cfile, new_server, domain);
    def_cont.len =
        snprintf(NULL, 0, cont_fmt, (int)domain.len, domain.data,
                 (int)domain.len, domain.data, (int)domain.len, domain.data,
                 (int)domain.len, domain.data, (int)domain.len, domain.data,
                 (int)domain.len, domain.data, (int)domain.len, domain.data
                 );
    def_cont.len += 1;
    def_cont.data = ngx_pcalloc(r->pool, def_cont.len);
    snprintf((char *)def_cont.data, def_cont.len, cont_fmt, (int)domain.len,
             domain.data, (int)domain.len, domain.data, (int)domain.len,
             domain.data, (int)domain.len, domain.data, (int)domain.len,
             domain.data,
             (int)domain.len, domain.data, (int)domain.len, domain.data
             );
    config_create_file((char *)cfile->data, (char *)def_cont.data,
                       (int)def_cont.len);

    const char *utype = ".upstream";
    new_server_config_file(r, utype, cfile, new_server, domain);

    ups_cont.len = snprintf(NULL,0,ups_fmt,(int)domain.len, domain.data);
    ups_cont.len +=1;
    ups_cont.data = ngx_pcalloc(r->pool,ups_cont.len);
    snprintf((char*)ups_cont.data,ups_cont.len,ups_fmt,(int)domain.len, domain.data);
    config_create_file((char *)cfile->data,(char*) ups_cont.data, (int)ups_cont.len);

    const char *ltype = ".location";
    new_server_config_file(r, ltype, cfile, new_server, domain);

    loc_cont.len = snprintf(NULL,0,loc_fmt,(int)domain.len, domain.data);
    loc_cont.len +=1;
    loc_cont.data = ngx_pcalloc(r->pool,loc_cont.len);
    snprintf((char*)loc_cont.data,loc_cont.len,loc_fmt,(int)domain.len, domain.data);

    config_create_file((char *)cfile->data, (char*)loc_cont.data,(int)loc_cont.len);

    const char *ptype = ".pem";
    new_server_config_file(r, ptype, cfile, new_server, domain);

    config_create_file((char *)cfile->data, "", strlen(loc_fmt));

    const char *ktype = ".key";
    new_server_config_file(r, ktype, cfile, new_server, domain);

    config_create_file((char *)cfile->data, "", strlen(loc_fmt));

    res = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
    ngx_str_set(res, "{\"status\":0}\0");
    post_body_header_out(r, res, 1, 1);
} /* -----  end of function ngx_cluster_new_server  ----- */

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_cluster_get_config Description:
 * =====================================================================================
 */
static void ngx_cluster_get_config(ngx_http_request_t *r) {
    ngx_open_file_info_t of;
    ngx_http_core_loc_conf_t *clcf;

    ngx_int_t rc;
    ngx_buf_t *b;
    ngx_chain_t out;
    ngx_str_t path, *base_dir;
    ngx_http_post_body_t *pb, *hp;

    ngx_http_cluster_loc_conf_t *lccf;

    lccf = ngx_http_get_module_loc_conf(r, ngx_http_cluster_module);
    if (!lccf) {
        NX_LOG("ngx_http_top_header_filter error");
        return;
    }

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    base_dir = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
    build_base_dir(r, base_dir, lccf);

    pb = ngx_pcalloc(r->pool, sizeof(ngx_http_post_body_t));
    hp = pb;
    post_body_add(r, hp, "path");

    post_body_data(r, pb);
    hp = pb;
    post_body_get(hp, "path");

    if (hp->data.len == 0) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
    path.len = hp->data.len + base_dir->len + 1;
    path.data = ngx_pcalloc(r->pool, path.len);
    if (path.data == NULL) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
    snprintf((char *)path.data, path.len, "%.*s/%.*s", (int)base_dir->len,
             base_dir->data, (int)(hp->data.len), hp->data.data);

    /** NX_DEBUG("path:%V", &path); */

    ngx_memzero(&of, sizeof(ngx_open_file_info_t));

    of.read_ahead = clcf->read_ahead;
    of.directio = clcf->directio;
    of.valid = clcf->open_file_cache_valid;
    of.min_uses = clcf->open_file_cache_min_uses;
    of.errors = clcf->open_file_cache_errors;
    of.events = clcf->open_file_cache_events;

    if (ngx_http_set_disable_symlinks(r, clcf, &path, &of) != NGX_OK) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
    if (ngx_open_cached_file(clcf->open_file_cache, &path, &of, r->pool) !=
        NGX_OK) {
        switch (of.err) {
            case 0:
                ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
                return;

            case NGX_ENOENT:
            case NGX_ENOTDIR:
            case NGX_ENAMETOOLONG:

                rc = NGX_HTTP_NOT_FOUND;
                break;

            case NGX_EACCES:
#if (NGX_HAVE_OPENAT)
            case NGX_EMLINK:
            case NGX_ELOOP:
#endif

                rc = NGX_HTTP_FORBIDDEN;
                break;

            default:

                rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
                break;
        }

        ngx_http_finalize_request(r, rc);
        return;
    }

    r->root_tested = !r->error_page;

    if (of.is_dir) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
#if !(NGX_WIN32) /* the not regular files are probably Unix specific */

    if (!of.is_file) {
        ngx_http_finalize_request(r, NGX_HTTP_NOT_FOUND);
        return;
    }

#endif

    rc = ngx_http_discard_request_body(r);

    if (rc != NGX_OK) {
        ngx_http_finalize_request(r, rc);
        return;
    }

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = of.size;
    r->headers_out.last_modified_time = of.mtime;

    if (ngx_http_set_etag(r) != NGX_OK) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    if (ngx_http_set_content_type(r) != NGX_OK) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
    if (r != r->main && of.size == 0) {
        ngx_http_send_header(r);
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);

        return;
    }

    r->allow_ranges = 1;

    /* we need to allocate all before the header would be sent */

    b = ngx_calloc_buf(r->pool);
    if (b == NULL) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    b->file = ngx_pcalloc(r->pool, sizeof(ngx_file_t));
    if (b->file == NULL) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }

    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        ngx_http_finalize_request(r, rc);
        return;
    }

    b->file_pos = 0;
    b->file_last = of.size;

    b->in_file = b->file_last ? 1 : 0;
    b->last_buf = (r == r->main) ? 1 : 0;
    b->last_in_chain = 1;
    b->file->fd = of.fd;
    b->file->name = path;
    b->file->directio = of.is_directio;

    out.buf = b;
    out.next = NULL;

    rc = ngx_http_output_filter(r, &out);
    ngx_http_finalize_request(r, rc);
    return;
} /* -----  end of function ngx_cluster_get_config  ----- */

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_cluster_upload_config Description:
 * =====================================================================================
 */
static void ngx_cluster_upload_config(ngx_http_request_t *r) {
    ngx_str_t *res;
    size_t flen;
    ngx_str_t dirpath;
    ngx_str_t path;

    ngx_str_t dest;
    ngx_str_t config_file, *base_dir;

    ngx_http_post_body_t *pb, *hp;

    ngx_str_t base64;

    ngx_http_cluster_loc_conf_t *lccf;

    lccf = ngx_http_get_module_loc_conf(r, ngx_http_cluster_module);
    if (!lccf) {
        NX_LOG("ngx_http_top_header_filter error");
        return;
    }

    pb = ngx_pcalloc(r->pool, sizeof(ngx_http_post_body_t));
    hp = pb;
    post_body_add(r, hp, "path");
    post_body_add(r, hp, "base64");
    post_body_data(r, pb);
    hp = pb;
    post_body_get(hp, "path");

    if (hp->data.len == 0) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
    config_file = hp->data;
    hp = pb;
    post_body_get(hp, "base64");

    if (hp->data.len == 0) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
    base64 = hp->data;

    /**     NX_DEBUG("base64 len:%d", base64->len); */
    /** NX_DEBUG("base64 all:%s", base64->data + (base64->len - 10)); */
    base64.len -= 1;
    dest.len = ngx_base64_decoded_length(base64.len);
    dest.data = ngx_pcalloc(r->pool, dest.len);
    ngx_int_t d = ngx_decode_base64(&dest, &base64);
    if (d == NGX_ERROR) {
        NX_DEBUG("d NULL");
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
    /** NX_DEBUG("DEST:%V", config_file); */
    base_dir = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
    build_base_dir(r, base_dir, lccf);

    path.len = base_dir->len + config_file.len + 1;
    path.data = ngx_pcalloc(r->pool, path.len);
    if (path.data == NULL) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
    snprintf((char *)path.data, path.len, "%.*s/%.*s", (int)base_dir->len,
             base_dir->data, (int)config_file.len, config_file.data);
    flen = nindex((char *)path.data, '/');
    dirpath.len = flen + 1;
    dirpath.data = ngx_pcalloc(r->pool, dirpath.len);
    snprintf((char *)dirpath.data, dirpath.len, "%s", path.data);

    /** NX_DEBUG("dirpath:%V ,path:%V",&dirpath,&path); */
    /** NX_DEBUG("dest:%V",&dest); */
    config_create_dir((char *)dirpath.data, 0755);
    config_create_file((char *)path.data, (char *)dest.data, dest.len);

    res = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
    ngx_str_set(res, "{\"status\":0}\0");
    post_body_header_out(r, res, 1, 1);

    return;
} /* -----  end of function ngx_cluster_upload_config  ----- */
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_cluster_delete_server Description:
 * =====================================================================================
 */
static void ngx_cluster_delete_server(ngx_http_request_t *r) {
    ngx_http_post_body_t *pb, *hp;
    ngx_str_t *base_dir, del_dir, del_files;
    ngx_str_t *res;
    char *dlist[] = {".conf", ".key", ".pem", "_upstream.conf",
                     "_location.conf"};
    char *plist;
    int m;
    size_t dlen;
    ngx_http_cluster_loc_conf_t *lccf;

    lccf = ngx_http_get_module_loc_conf(r, ngx_http_cluster_module);
    if (!lccf) {
        NX_LOG("ngx_http_top_header_filter error");
        return;
    }

    pb = ngx_pcalloc(r->pool, sizeof(ngx_http_post_body_t));
    hp = pb;
    post_body_add(r, hp, "domain");
    post_body_data(r, pb);
    hp = pb;
    post_body_get(hp, "domain");

    if (hp->data.len == 0) {
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
    }
    base_dir = ngx_pcalloc(r->pool, sizeof(ngx_str_t));

    build_base_dir(r, base_dir, lccf);

    dlen = nindex((char *)hp->data.data, '/');

    /** NX_DEBUG("domain:%V,base_dir:%V", &hp->data, base_dir); */
    del_dir.len = snprintf(NULL, 0, "%.*s/%.*s", (int)base_dir->len,
                           base_dir->data, (int)hp->data.len, hp->data.data);
    del_dir.len += 1;
    del_dir.data = ngx_pcalloc(r->pool, del_dir.len);
    snprintf((char *)del_dir.data, del_dir.len, "%.*s/%.*s", (int)base_dir->len,
             base_dir->data, (int)hp->data.len, hp->data.data);
    res = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
    ngx_str_set(res, "{\"status\":-1}\0");
    for (m = 0; m < 5; m++) {
        plist = dlist[m];

        del_files.len =
            snprintf(NULL, 0, "%.*s/%.*s/%.*s%s", (int)base_dir->len,
                     base_dir->data, (int)hp->data.len, hp->data.data,
                     (int)(hp->data.len - dlen), hp->data.data + dlen, plist);
        del_files.len += 1;
        del_files.data = ngx_pcalloc(r->pool, del_files.len);
        snprintf((char *)del_files.data, del_files.len, "%.*s/%.*s/%.*s%s",
                 (int)base_dir->len, base_dir->data, (int)hp->data.len,
                 hp->data.data, (int)(hp->data.len - dlen),
                 hp->data.data + dlen, plist);
        NX_DEBUG("del files:%V", &del_files);

        if (ngx_delete_file((char *)del_files.data) == NGX_FILE_ERROR) {
            NX_DEBUG("error");

            post_body_header_out(r, res, 1, 1);
            return;
        }
    }

    if (ngx_delete_dir(del_dir.data) != NGX_FILE_ERROR) {
        NX_DEBUG("delete ok");
    } else {
        NX_DEBUG("delete error");
    }
    ngx_str_set(res, "{\"status\":0}\0");
    post_body_header_out(r, res, 1, 1);
    return;
} /* -----  end of function ngx_cluster_delete_server  ----- */

/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_http_cluster_rest Description:
 * =====================================================================================
 */
static ngx_int_t ngx_http_cluster_rest(ngx_http_request_t *r) {
    ngx_str_t uri = get_uri(r);
    char *p = index((char *)uri.data + 1, '/');

    const char *upload = "/upload/config";
    const char *get_config = "/get/config";
    const char *get_list = "/get/list";
    const char *commit = "/commit/config";
    const char *new_server = "/new/server";
    const char *delete_server = "/delete/server";

    if (ngx_strncmp(p, upload, strlen(upload)) == 0) {
        ngx_http_read_client_request_body(r, ngx_cluster_upload_config);
        return NGX_DONE;
    } else if (ngx_strncmp(p, get_config, strlen(get_config)) == 0) {
        ngx_http_read_client_request_body(r, ngx_cluster_get_config);
        return NGX_DONE;
    } else if (ngx_strncmp(p, get_list, strlen(get_list)) == 0) {
        ngx_http_read_client_request_body(r, ngx_cluster_list_config);
        return NGX_DONE;
    } else if (ngx_strncmp(p, commit, strlen(commit)) == 0) {
        ngx_http_read_client_request_body(r, ngx_cluster_commit_config);
        return NGX_DONE;
    } else if (ngx_strncmp(p, new_server, strlen(new_server)) == 0) {
        ngx_http_read_client_request_body(r, ngx_cluster_new_server);
        return NGX_DONE;
    } else if (ngx_strncmp(p, delete_server, strlen(delete_server)) == 0) {
        ngx_http_read_client_request_body(r, ngx_cluster_delete_server);
        return NGX_DONE;
    }

    return NGX_ERROR;
} /* -----  end of function ngx_http_cluster_rest  ----- */
