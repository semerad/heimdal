#include "krb5_locl.h"

RCSID("$Id$");

/* Public principal handling functions */

void
krb5_free_principal(krb5_context context,
		    krb5_principal p)
{
    int i;
    for(i = 0; i < p->ncomp; i++)
	krb5_data_free(&p->comp[i]);
    free(p->comp);
    krb5_data_free(&p->realm);
    free(p);
}

krb5_error_code
krb5_parse_name(krb5_context context,
		const char *name,
		krb5_principal *principal)
{

    krb5_data *comp;
    int ncomp;
    krb5_data realm;

    char *p;
    char *q;
    char *s;
    char *start;

    int n;
    char c;
    int got_realm = 0;
  
    /* count number of component */
    ncomp = 1;
    for(p = (char*)name; *p; p++){
	if(*p=='\\'){
	    if(!p[1])
		return KRB5_PARSE_MALFORMED;
	    p++;
	} else if(*p == '/')
	    ncomp++;
    }
    comp = ALLOC(ncomp, krb5_data);
  
    n = 0;
    start = q = p = s = strdup(name);
    while(*p){
	c = *p++;
	if(c == '\\'){
	    c = *p++;
	    if(c == 'n')
		c = '\n';
	    else if(c == 't')
		c = '\t';
	    else if(c == 'b')
		c = '\b';
	    else if(c == '0')
		c = '\0';
	}else if(c == '/' || c == '@'){
	    if(got_realm){
		while(n>0)
		    free(comp[--n].data);
		free(comp);
		free(s);
		return KRB5_PARSE_MALFORMED;
	    }else{
		comp[n].length = q - start;
		comp[n].data = (krb5_pointer)malloc(comp[n].length);
		memmove(comp[n].data, start, comp[n].length);
		n++;
	    }
	    if(c == '@')
		got_realm = 1;
	    start = q;
	    continue;
	}
	if(got_realm && (c == ':' || c == '/' || c == '\0')){
	    while(n>0)
		free(comp[--n].data);
	    free(comp);
	    free(s);
	    return KRB5_PARSE_MALFORMED;
	}
	*q++ = c;
    }
    if(got_realm){
	realm.length = q - start;
	realm.data = (krb5_pointer)malloc(realm.length);
	memmove(realm.data, start, realm.length);
    }else{
	comp[n].length = q - start;
	comp[n].data = (krb5_pointer)malloc(comp[n].length);
	memmove(comp[n].data, start, comp[n].length);
	n++;
    }
    *principal = ALLOC(1, krb5_principal_data);
    (*principal)->type = KRB5_NT_PRINCIPAL;
    (*principal)->realm = realm;
    (*principal)->comp = comp;
    (*principal)->ncomp = n;
    free(s);
    return 0;
}

static void quote_string(char *s, int len, char **out)
{
    char *q;
    char *p = *out;
    int c=0;
    for(q = s; q < s + len; q++){
	if(*q == '\n')
	    c = 'n';
	else if(*q == '\t')
	    c = 't';
	else if(*q == '\b')
	    c = 'b';
	else if(*q == '\0')
	    c = '0';
	else if(*q == '/')
	    c='/';      
	else if(*q == '@')
	    c = '@';
	if(c){
	    *p++ = '\\';
	    *p++ = c;
	    c = 0;
	}else
	    *p++ = *q;
    }
    *out = p;
}


krb5_error_code
krb5_unparse_name(krb5_context context,
		  krb5_principal principal,
		  char **name)
{
    int size = 0;
    char *p;
    char *s;
    int i;
    for(i = 0; i < principal->ncomp; i++)
	size += 2*principal->comp[i].length + 1;
    size += 2*principal->realm.length + 1;
    s = ALLOC(size, char);
    p = s;
    for(i = 0; i < principal->ncomp; i++){
	if(i) *p++ = '/';
	quote_string(principal->comp[i].data, principal->comp[i].length, &p);
    }
    *p++ = '@';
    quote_string(principal->realm.data, principal->realm.length, &p);
    *p = 0;
    *name = strdup(s);
    free(s);
    return 0;
}


krb5_error_code
krb5_unparse_name_ext(krb5_context context,
		      krb5_const_principal principal,
		      char **name,
		      size_t *size)
{
    fprintf(stderr, "krb5_unparse_name_ext: not implemented\n");
    abort();
}


krb5_data*
krb5_princ_realm(krb5_context context,
		 krb5_principal principal)
{
    return &principal->realm;
}


void
krb5_princ_set_realm(krb5_context context,
		     krb5_principal principal,
		     krb5_data *realm)
{
    principal->realm = *realm;
}


krb5_error_code
krb5_build_principal(krb5_context context,
		     krb5_principal *principal,
		     int rlen,
		     const char *realm,
		     ...)
{
    krb5_error_code ret;
    va_list ap;
    va_start(ap, realm);
    ret = krb5_build_principal_va(context, principal, rlen, realm, ap);
    va_end(ap);
    return ret;
}

krb5_error_code
krb5_principal_set_component(krb5_context context, krb5_principal p, 
			     int n, void *data, size_t len)
{
    krb5_data *tmp;
    if(p->ncomp <= n){
	int s = n + 10;
	if(p->comp)
	    tmp = (krb5_data*)realloc(p->comp, s * sizeof(krb5_data));
	else
	    tmp = ALLOC(s, krb5_data);
	if(!tmp)
	    return ENOMEM;
	p->comp = tmp;
	p->ncomp = s;
    }
    p->comp[n].length = 0;
    p->comp[n].data = NULL;
    krb5_data_copy(&p->comp[n], data, len);
    return 0;
}

static void
va_ext_princ(krb5_context context, krb5_principal p, va_list ap)
{
    int n = 0;
    while(1){
	char *s;
	int len;
	len  = va_arg(ap, int);
	if(len == 0)
	    break;
	s = va_arg(ap, char*);
	krb5_principal_set_component(context, p, n, s, len);
	n++;
    }
    p->ncomp = n;
}

static void
va_princ(krb5_context context, krb5_principal p, va_list ap)
{
    int n = 0;
    while(1){
	char *s;
	int len;
	s = va_arg(ap, char*);
	if(s == NULL)
	    break;
	len = strlen(s);
	krb5_principal_set_component(context, p, n, s, len);
	n++;
    }
    p->ncomp = n;
}


static krb5_error_code
build_principal(krb5_context context,
		krb5_principal *principal,
		int rlen,
		const char *realm,
		void (*func)(krb5_context, krb5_principal, va_list),
		va_list ap)
{
    krb5_principal p;
    int n;
  
    p = calloc(1, sizeof(*p));
    if (p == NULL)
	return ENOMEM;
    p->type = KRB5_NT_PRINCIPAL;

    if(krb5_data_copy(&p->realm, (void*)realm, rlen)){
	free(p);
	return ENOMEM;
    }
  
    (*func)(context, p, ap);
    *principal = p;
    return 0;
}

krb5_error_code
krb5_build_principal_va(krb5_context context, 
			krb5_principal *principal, 
			int rlen,
			const char *realm,
			va_list ap)
{
    return build_principal(context, principal, rlen, realm, va_princ, ap);
}

/* Not part of MIT K5 API */
krb5_error_code
krb5_build_principal_va_ext(krb5_context context, 
			    krb5_principal *principal, 
			    int rlen,
			    const char *realm,
			    va_list ap)
{
    return build_principal(context, principal, rlen, realm, va_ext_princ, ap);
}


krb5_error_code
krb5_build_principal_ext(krb5_context context,
			 krb5_principal *principal,
			 int rlen,
			 const char *realm,
			 ...)
{
    krb5_error_code ret;
    va_list ap;
    va_start(ap, realm);
    ret = krb5_build_principal_va_ext(context, principal, rlen, realm, ap);
    va_end(ap);
    return ret;
}


krb5_error_code
krb5_copy_principal(krb5_context context,
		    krb5_const_principal inprinc,
		    krb5_principal *outprinc)
{
    krb5_principal p;
    int i;
    p = calloc(1, sizeof(*p));
    if (p == NULL)
	return ENOMEM;
    p->type = inprinc->type;
    if(krb5_data_copy(&p->realm, inprinc->realm.data, inprinc->realm.length)){
	krb5_free_principal(context, p);
	return ENOMEM;
    }
    p->comp = ALLOC(inprinc->ncomp, krb5_data);
    if(!p->comp){
	krb5_free_principal(context, p);
	return ENOMEM;
    }
  
    for(i=0; i<inprinc->ncomp; i++){
	p->comp[i].length = 0;
	if(krb5_data_copy(&p->comp[i], inprinc->comp[i].data, 
			  inprinc->comp[i].length)){
	    krb5_free_principal(context, p);
	    return ENOMEM;
	}
	p->ncomp = i+1;
    }
    *outprinc = p;
    return 0;
}


krb5_boolean
krb5_principal_compare(krb5_context context,
		       krb5_const_principal princ1,
		       krb5_const_principal princ2)
{
    int i;
    if(!krb5_realm_compare(context, princ1, princ2))
	return FALSE;
    if(princ1->ncomp != princ2->ncomp)
	return FALSE;
    for(i=0; i<princ1->ncomp; i++){
	if(princ1->comp[i].length != princ2->comp[i].length)
	    return FALSE;
	if(memcmp(princ1->comp[i].data, princ2->comp[i].data, 
		  princ1->comp[i].length))
	    return FALSE;
    }
    return TRUE;
}


krb5_boolean
krb5_realm_compare(krb5_context context,
		   krb5_const_principal princ1,
		   krb5_const_principal princ2)
{
    if(princ1->realm.length != princ2->realm.length)
	return FALSE;
    if(memcmp(princ1->realm.data, princ2->realm.data, princ1->realm.length))
	return FALSE;
    return TRUE;
}

		   
krb5_error_code
krb5_425_conv_principal(krb5_context context,
			const char *name,
			const char *instance,
			const char *realm,
			krb5_principal *princ)
{
    if(!strcmp(name, "rcmd"))
	name = "host";
    return krb5_build_principal(context, princ, 
				strlen(realm), realm, name, instance, 0);
}


			
krb5_error_code
krb5_sname_to_principal (krb5_context context,
			 const char *hostname,
			 const char *sname,
			 int32_t type,
			 krb5_principal *ret_princ)
{
    krb5_error_code ret;
    char **r;

    ret = krb5_get_host_realm (context, hostname, &r);
    if (ret)
	return ret;
    
    return krb5_build_principal (context,
				 ret_princ,
				 strlen(r[0]),
				 r[0],
				 sname,
				 hostname,
				 0);
}
