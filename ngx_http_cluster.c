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

#include "ngx_cluster.h"

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

static char *ngx_http_cluster_web(ngx_conf_t *cf, ngx_command_t *cmd,
                                  void *conf);
static ngx_int_t ngx_http_cluster_web_handler(ngx_http_request_t *r);

/**
 *  branch function
 */

static char *ngx_http_branch(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
/**
 * This module provided directive: cluster.
 *
 */
static ngx_command_t ngx_http_cluster_commands[] = {
    {ngx_string(NGX_CLUSTER_MAIN), NGX_HTTP_MAIN_CONF | NGX_CONF_FLAG,
     ngx_conf_set_flag_slot, NGX_HTTP_LOC_CONF_OFFSET,
     offsetof(ngx_http_cluster_loc_conf_t, ngx_cluster_main), NULL},

    {ngx_string(NGX_CLUSTER_BRANCH), NGX_HTTP_MAIN_CONF | NGX_CONF_NOARGS,
     ngx_http_branch, NGX_HTTP_LOC_CONF_OFFSET, 0, NULL},

    {ngx_string(NGX_CLUSTER_NODE), NGX_HTTP_MAIN_CONF | NGX_CONF_FLAG,
     ngx_conf_set_flag_slot, NGX_HTTP_LOC_CONF_OFFSET,
     offsetof(ngx_http_cluster_loc_conf_t, ngx_cluster_node), NULL},

    {ngx_string("cluster_api"),           /* directive */
     NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS, /* location context and takes
                                             no arguments*/
     ngx_http_cluster_web,                /* configuration setup function */
     0, /* No offset. Only one context is supported. */
     0, /* No offset when storing the module configuration on struct. */
     NULL},
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
    conf->ngx_cluster_main = NGX_CONF_UNSET_UINT;
    conf->ngx_cluster_node = NGX_CONF_UNSET_UINT;

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

    ngx_conf_merge_value(conf->ngx_cluster_main, prev->ngx_cluster_main, 0);
    ngx_conf_merge_value(conf->ngx_cluster_node, prev->ngx_cluster_node, 0);

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
    lccf = ngx_http_get_module_loc_conf(r, ngx_http_cluster_module);
    if (!lccf) {
        NX_LOG("ngx_http_top_header_filter error");
        return NGX_DECLINED;
    }
    if (lccf->ngx_cluster_main == 0) {
        return NGX_DECLINED;
    }
    if (r != r->main) {
        return NGX_DECLINED;
    }
    NX_LOG("ngx_cluster_main:%d", lccf->ngx_cluster_main);

    if (lccf->ngx_cluster_branch == NULL) {
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

    name = mlcf->cluster->elts;

    for (i = 0; i < mlcf->cluster->nelts; i++) {
        if (ngx_http_subrequest(r, &name[i], &r->args, &sr, NULL,
                                NGX_HTTP_SUBREQUEST_BACKGROUND) != NGX_OK) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        sr->header_only = 1;
        sr->method = r->method;
        sr->method_name = r->method_name;
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
 * ===  FUNCTION
 * ====================================================================== Name:
 * ngx_http_cluster_web Description:
 * =====================================================================================
 */
static char *ngx_http_cluster_web(ngx_conf_t *cf, ngx_command_t *cmd,
                                  void *conf) {
    ngx_http_core_loc_conf_t *clcf; /* pointer to core location configuration */

    /* Install the hello world handler. */
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
    /* Get the content length of the body. */
    r->headers_out.content_length_n = sizeof(ngx_web_cluster) - 1;
    ngx_http_send_header(r); /* Send the headers */

    /* Send the body, and return the status code of the output filter chain. */
    return ngx_http_output_filter(r, &out);

} /* -----  end of function ngx_http_cluster_web_handler  ----- */
