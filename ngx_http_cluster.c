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
#include <libgen.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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
static char *ngx_http_cluster_node(ngx_conf_t *cf, ngx_command_t *cmd,
                                   void *conf);
static ngx_int_t ngx_http_cluster_node_handler(ngx_http_request_t *r);
/**
 *  branch function
 */

static char *ngx_http_branch(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void ngx_http_cluster_body_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_cluster_handler_internal(ngx_http_request_t *r);

ngx_str_t get_uri(ngx_http_request_t *r);
ngx_table_elt_t *search_headers_in(ngx_http_request_t *r, u_char *name,
                                   size_t len);
int config_create_dir(const char *path, const mode_t mode);
void config_create_file(const char *path, const char *cont, int len);
size_t nindex(const char *s, int c);

/**
 * web route api;
 */
ngx_str_t *web_route_upload_conf(ngx_http_request_t *r,
                                 ngx_http_cluster_loc_conf_t *lccf);
/**
 * This module provided directive: cluster.
 *
 */
static ngx_command_t ngx_http_cluster_commands[] = {
    {ngx_string(NGX_CLUSTER_MAIN), NGX_HTTP_MAIN_CONF | NGX_CONF_FLAG,
     ngx_conf_set_flag_slot, NGX_HTTP_LOC_CONF_OFFSET,
     offsetof(ngx_http_cluster_loc_conf_t, ngx_cluster_main), NULL},

    {ngx_string(NGX_CLUSTER_BRANCH),
     NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF |
         NGX_CONF_TAKE1,
     ngx_http_branch, NGX_HTTP_LOC_CONF_OFFSET, 0, NULL},

    {ngx_string(NGX_CLUSTER_NODE), NGX_HTTP_LOC_CONF | NGX_CONF_NOARGS,
     ngx_http_cluster_node, 0, 0, NULL},

    {ngx_string(NGX_WEB_API),           /* directive */
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
    /** conf->ngx_cluster_node = NGX_CONF_UNSET_UINT; */
    conf->ngx_cluster_branch = NGX_CONF_UNSET_PTR;
    conf->confname = NGX_CONF_UNSET_PTR;
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
    /** ngx_conf_merge_value(conf->ngx_cluster_node, prev->ngx_cluster_node, 0); */
    ngx_conf_merge_ptr_value(conf->ngx_cluster_branch, prev->ngx_cluster_branch,
                             NULL);
    ngx_conf_merge_ptr_value(conf->confname, prev->confname,
                             cf->conf_file->file.name.data);
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
    if (lccf->ngx_cluster_main == 0) {
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
    ngx_buf_t *b;
    ngx_chain_t out;
    size_t html_json = 0;
    static u_char ngx_node_default_out[] = "{\"status\":-1}";
    ngx_str_t uri = get_uri(r);
    ngx_str_t *resData;
    ngx_http_cluster_loc_conf_t *lccf;

    lccf = ngx_http_get_module_loc_conf(r, ngx_http_cluster_module);

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    out.buf = b;
    out.next = NULL;
    resData = NULL;

    if (ngx_strcmp(uri.data, "/upload_conf") == 0) {
        html_json = 1;
        resData = web_route_upload_conf(r, lccf);
    }
  /**   else if (ngx_strcmp(uri.data, "/check_conf") == 0) { */
    /** } */

    if (resData == NULL) {
        resData = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
        resData->len = strlen((char *)ngx_node_default_out) + 1;
        resData->data = ngx_pcalloc(r->pool, resData->len);
        snprintf((char *)resData->data, resData->len, "%s",
                 ngx_node_default_out);
    }
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

    /* Send the body, and return the status code of the output filter chain. */
    return ngx_http_output_filter(r, &out);
} /* -----  end of function ngx_http_cluster_node_handler  ----- */
/*
 * ===  FUNCTION
 * ====================================================================== Name:
 * web_route_upload_conf Description:
 * =====================================================================================
 */
ngx_str_t *web_route_upload_conf(ngx_http_request_t *r,
                                 ngx_http_cluster_loc_conf_t *lccf) {
    ngx_table_elt_t *phv, *chv;
    const char *path = "path", *ngx_conf = "ngxconf";
    char *confdir = NULL, *file_name;
    const char *fmt = "%.*s%.*s";
    size_t cdlen = 0, clen, v, flen;
    ngx_str_t dest;
    ngx_str_t *res;
    static u_char ok_out[] = "{\"status\":0}";

    phv = search_headers_in(r, (u_char *)path, strlen(path));
    if (phv == NULL || phv->value.len == 0) {
        NX_DEBUG("PATH");
        return NULL;
    }
    chv = search_headers_in(r, (u_char *)ngx_conf, strlen(ngx_conf));
    if (chv == NULL || chv->value.len == 0) {
        NX_DEBUG("ngxconf");
        return NULL;
    }
    if (*phv->value.data == '.') {
        clen = nindex((char *)lccf->confname, '/');
        clen += 1;
        v = nindex((char *)phv->value.data, '/');
        v -= 1;
        cdlen = snprintf(NULL, 0, fmt, clen, lccf->confname, v,
                         (phv->value.data + 2));
        confdir = ngx_pcalloc(r->pool, cdlen);
        if (confdir == NULL) {
            return NULL;
        }
        snprintf(confdir, cdlen, fmt, clen, lccf->confname, v,
                 (phv->value.data + 2));
    } else if (*phv->value.data == '/') {
        /** NX_DEBUG("//"); */
        cdlen = nindex((char *)phv->value.data, '/');
        cdlen += 1;
        confdir = ngx_pcalloc(r->pool, cdlen);
        if (confdir == NULL) {
            return NULL;
        }
        snprintf(confdir, cdlen, "%s", phv->value.data);

    } else {
        NX_DEBUG("bug path");
        return NULL;
    }
    char *p = rindex((char *)phv->value.data, '/');
    flen = snprintf(NULL, 0, "%s%s", confdir, p);
    flen += 1;
    file_name = ngx_pcalloc(r->pool, flen);
    snprintf(file_name, flen, "%s%s", confdir, p);

    if (config_create_dir(confdir, 0755) == -1) {
        NX_DEBUG("con't create dir");
    }

    dest.len = ngx_base64_decoded_length(chv->value.len);
    dest.data = ngx_pcalloc(r->pool, dest.len);
    ngx_int_t d = ngx_decode_base64(&dest, &chv->value);
    if (d == NGX_ERROR) {
        return NULL;
    }
    config_create_file(file_name, (char *)dest.data, dest.len);

    ngx_pfree(r->pool, confdir);
    ngx_pfree(r->pool, file_name);

    res = ngx_pcalloc(r->pool, sizeof(ngx_str_t));
    ngx_str_set(res, ok_out);
    res->len += 1;

    return res;
} /* -----  end of function web_route_upload_conf  ----- */
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
 * ====================================================================== Name:
 * search_headers_in Description:
 * =====================================================================================
 */
ngx_table_elt_t *search_headers_in(ngx_http_request_t *r, u_char *name,
                                   size_t len) {
    ngx_list_part_t *part;
    ngx_table_elt_t *h;
    ngx_uint_t i;

    /*
       Get the first part of the list. There is usual only one part.
       */
    part = &r->headers_in.headers.part;
    h = part->elts;

    /*
       Headers list array may consist of more than one part,
       so loop through all of it
       */
    for (i = 0; /* void */; i++) {
        if (i >= part->nelts) {
            if (part->next == NULL) {
                /* The last part, search is done. */
                break;
            }

            part = part->next;
            h = part->elts;
            i = 0;
        }

        /*
           Just compare the lengths and then the names case insensitively.
           */
        if (len != h[i].key.len || ngx_strcasecmp(name, h[i].key.data) != 0) {
            /* This header doesn't match. */
            continue;
        }

        /*
           Ta-da, we got one!
           Note, we'v stop the search at the first matched header
           while more then one header may fit.
           */
        return &h[i];
    }

    /*
       No headers was found
       */
    return NULL;

} /* -----  end of function search_headers_in  ----- */

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
    fclose(fp);
} /* -----  end of function config_create_file  ----- */

