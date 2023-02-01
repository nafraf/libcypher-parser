/* vi:set ts=4 sw=4 expandtab:
 *
 * Copyright 2016, Chris Leishman (http://github.com/cleishm)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "../../config.h"
#include "astnode.h"
#include "util.h"
#include <assert.h>


struct call_subquery
{
    cypher_astnode_t _astnode;
    unsigned int nclauses;
    const cypher_astnode_t *clauses[];
};


static cypher_astnode_t *clone(const cypher_astnode_t *self,
        cypher_astnode_t **children);
static ssize_t detailstr(const cypher_astnode_t *self, char *str, size_t size);
static void call_release(cypher_astnode_t *self);


static const struct cypher_astnode_vt *parents[] =
    { &cypher_query_clause_astnode_vt };

const struct cypher_astnode_vt cypher_call_subquery_astnode_vt =
    { .parents = parents,
      .nparents = 1,
      .name = "CALL SUBQUERY",
      .detailstr = detailstr,
      .release = cypher_astnode_release,
      .clone = clone };


cypher_astnode_t *cypher_ast_call_subquery(cypher_astnode_t * const *clauses,
        unsigned int nclauses, cypher_astnode_t **children,
        unsigned int nchildren, struct cypher_input_range range)
{
    REQUIRE_CHILD_ALL(children, nchildren, clauses, nclauses,
            CYPHER_AST_QUERY_CLAUSE, NULL);

    struct call_subquery *node = calloc(1, sizeof(struct call_subquery) +
            nclauses * sizeof(cypher_astnode_t *));
    if (node == NULL)
    {
        return NULL;
    }
    if (cypher_astnode_init(&(node->_astnode), CYPHER_AST_CALL_SUBQUERY,
            children, nchildren, range))
    {
        goto cleanup;
    }
    memcpy(node->clauses, clauses,
            nclauses * sizeof(cypher_astnode_t *));
    node->nclauses = nclauses;
    return &(node->_astnode);

    int errsv;
cleanup:
    errsv = errno;
    free(node);
    errno = errsv;
    return NULL;
}


cypher_astnode_t *clone(const cypher_astnode_t *self,
        cypher_astnode_t **children)
{
    REQUIRE_TYPE(self, CYPHER_AST_CALL_SUBQUERY, NULL);
    struct call_subquery *node = container_of(self, struct call_subquery, _astnode);

    cypher_astnode_t **clauses =
            calloc(node->nclauses, sizeof(cypher_astnode_t *));
    if (clauses == NULL)
    {
        return NULL;
    }
    for (unsigned int i = 0; i < node->nclauses; ++i)
    {
        clauses[i] = children[child_index(self, node->clauses[i])];
    }

    cypher_astnode_t *clone = cypher_ast_call_subquery(
            clauses, node->nclauses,
            children, self->nchildren, self->range);
    int errsv = errno;
    free(clauses);
    errno = errsv;
    return clone;
}

const cypher_astnode_t *cypher_ast_call_subquery_get_nclauses(
        const cypher_astnode_t *astnode, uint ind)
{
    REQUIRE_TYPE(astnode, CYPHER_AST_CALL_SUBQUERY, NULL);
    struct call_subquery *node = container_of(astnode, struct call_subquery, _astnode);
    return node->nclauses;
}


const cypher_astnode_t *cypher_ast_call_subquery_get_clause(
        const cypher_astnode_t *astnode, uint ind)
{
    REQUIRE_TYPE(astnode, CYPHER_AST_CALL_SUBQUERY, NULL);
    struct call_subquery *node = container_of(astnode, struct call_subquery, _astnode);
    return node->clauses[ind];
}

ssize_t detailstr(const cypher_astnode_t *self, char *str, size_t size)
{
    REQUIRE_TYPE(self, CYPHER_AST_CALL_SUBQUERY, -1);
    struct call_subquery *node = container_of(self, struct call_subquery, _astnode);

    size_t n = 0;

    if (node->nclauses > 0)
    {
        n = snprint_sequence(str, size,
                node->clauses, node->nclauses);
        if (n < 0)
        {
            return -1;
        }
    }

    return n;
}
