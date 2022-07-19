/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#define USING_LOG_PREFIX SQL_RESV
#include "sql/resolver/cmd/ob_help_resolver.h"
#include "share/inner_table/ob_inner_table_schema.h"
#include "share/schema/ob_priv_type.h"
#include "share/schema/ob_schema_utils.h"
#include "sql/session/ob_sql_session_info.h"
#include "sql/ob_sql_context.h"
#include "sql/parser/ob_parser.h"
#include "lib/charset/ob_charset.h"
#include "observer/ob_server_struct.h"
#include "observer/ob_inner_sql_connection_pool.h"
#include "observer/ob_inner_sql_result.h"

using namespace oceanbase::common;
using namespace oceanbase::common::sqlclient;
using namespace oceanbase::share;
using namespace oceanbase::observer;
using namespace oceanbase::share::schema;
namespace oceanbase {
namespace sql {

#define ROW_NUM(sql_result, row_num)      \
  for (; OB_SUCC(sql_result->next());) {} \
  row_num = sql_result->result_set().get_return_rows();

ObHelpResolver::ObHelpResolver(ObResolverParams& params) : ObSelectResolver(params)
{}

ObHelpResolver::~ObHelpResolver()
{}

int ObHelpResolver::parse_and_resolve_select_sql(const ObString& select_sql)
{
  int ret = OB_SUCCESS;
  // 1. parse and resolve view defination
  if (OB_ISNULL(session_info_) || OB_ISNULL(params_.allocator_)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("data member is not init", K(ret), K(session_info_), K(params_.allocator_));
  } else {
    ParseResult select_result;
    ObParser parser(*params_.allocator_, session_info_->get_sql_mode());
    if (OB_FAIL(parser.parse(select_sql, select_result))) {
      LOG_WARN("parse select sql failed", K(select_sql), K(ret));
    } else {
      // use alias to make all columns number continued
      if (OB_ISNULL(select_result.result_tree_)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("result tree is NULL", K(ret));
      } else if (OB_UNLIKELY(
                     select_result.result_tree_->num_child_ != 1 || NULL == select_result.result_tree_->children_)) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("result tree is invalid",
            K(ret),
            K(select_result.result_tree_->num_child_),
            K(select_result.result_tree_->children_));
      } else if (OB_UNLIKELY(NULL == select_result.result_tree_->children_[0])) {
        ret = OB_ERR_UNEXPECTED;
        LOG_WARN("result tree is invalid", K(ret), "child ptr", select_result.result_tree_->children_[0]);
      } else {
        ParseNode* select_stmt_node = select_result.result_tree_->children_[0];
        if (OB_FAIL(ObSelectResolver::resolve(*select_stmt_node))) {
          LOG_WARN("resolve select in view definition failed", K(ret), K(select_stmt_node));
        }
      }
    }
  }

  return ret;
}

int ObHelpResolver::resolve(const ParseNode& parse_tree)
{
  LOG_INFO("[gq] begin ObHelpResolver");
  int ret = OB_SUCCESS;
  /* transform into select_sql */
  ObString select_sql;
  const char* mask = parse_tree.str_value_;
  const int len = parse_tree.str_len_;
  LOG_INFO("[gq] ObHelpResolver mask: ", K(mask), K(len));
  ObSqlStrGenerator sql_gen;
  if (OB_FAIL(sql_gen.init(params_.allocator_))) {
    LOG_WARN("fail to init sql string generator", K(ret));
  };
  if (parse_tree.type_ == T_HELP) {
    /* Inner Sql */
    common::ObMySQLProxy* sql_proxy = GCTX.sql_proxy_;
    ObSqlString sql;
    int64_t topic_num, category_num, keyword_num;
    int64_t help_category_id, help_keyword_id;
    ObString help_category_name;
    SMART_VAR(ObMySQLProxy::MySQLResult, res)
    {
      observer::ObInnerSQLResult* sql_result = NULL;
      ObISQLConnection* conn = NULL;
      ObInnerSQLConnectionPool* pool = NULL;
      if (OB_FAIL(sql.append_fmt("SELECT name, description, example from oceanbase.__all_help_topic"
                                 " where name = \"%.*s\"",
              len,
              mask))) {
        LOG_WARN("append sql failed", K(ret), K(sql));
      } else if (OB_ISNULL(pool = static_cast<ObInnerSQLConnectionPool*>(sql_proxy->get_pool()))) {
        ret = OB_NOT_INIT;
        LOG_WARN("connection pool is NULL", K(ret));
      } else if (OB_FAIL(pool->acquire(session_info_, conn))) {
        LOG_WARN("failed to acquire inner connection", K(ret));
      } else if (OB_FAIL(conn->execute_read(session_info_->get_effective_tenant_id(), sql.ptr(), res, true, false))) {
        LOG_WARN("execute sql failed", K(ret), K(sql));
      } else {
        if (OB_ISNULL(sql_result = dynamic_cast<observer::ObInnerSQLResult*>(res.get_result()))) {
          LOG_WARN("result is null", K(ret));
        } else {
          if (OB_SUCC(sql_result->next())) {};
          ROW_NUM(sql_result, topic_num);
          // sql_result->print_info();
          // topic_num = sql_result->result_set().get_return_rows(); // 这个get_return_rows返回的结果竟然是next了几次
          LOG_INFO("Inner select from oceanbase.__all_help_topic", K(sql_result->result_set()));
          if (topic_num == 0) {
            /* search oceanbase.__all_help_topic and oceanbase.__all_help_category */
            if (OB_FAIL(sql.assign_fmt(
                    "select help_keyword_id from oceanbase.__all_help_keyword where name = \"%.*s\"", len, mask))) {
              LOG_WARN("append sql failed", K(ret), K(sql));
            } else if (OB_FAIL(
                           conn->execute_read(session_info_->get_effective_tenant_id(), sql.ptr(), res, true, false))) {
              LOG_WARN("execute sql failed", K(ret), K(sql));
            } else {
              if (OB_ISNULL(sql_result = dynamic_cast<observer::ObInnerSQLResult*>(res.get_result()))) {
                LOG_WARN("result is null", K(ret));
              } else {
                LOG_INFO("Inner select from oceanbase.__all_help_keyword", K(sql_result->result_set()));
                if (OB_SUCC(sql_result->next()))
                  sql_result->get_int("help_keyword_id", help_keyword_id);
                ROW_NUM(sql_result, keyword_num);
                if (keyword_num == 0) {
                  topic_num = 0;
                } else {
                  LOG_INFO("[gq]help_keyword_id : ", K(help_keyword_id));
                  if (OB_FAIL(sql.assign_fmt(
                          "select name from oceanbase.__all_help_topic"
                          " join oceanbase.__all_help_relation on oceanbase.__all_help_topic.help_topic_id ="
                          " oceanbase.__all_help_relation.help_topic_id"
                          " where oceanbase.__all_help_relation.help_keyword_id = %ld",
                          help_keyword_id))) {
                    LOG_WARN("append sql failed", K(ret), K(sql));
                  } else if (OB_FAIL(conn->execute_read(
                                 session_info_->get_effective_tenant_id(), sql.ptr(), res, true, false))) {
                    LOG_WARN("execute sql failed", K(ret), K(sql));
                  } else {
                    if (OB_ISNULL(sql_result = dynamic_cast<observer::ObInnerSQLResult*>(res.get_result()))) {
                      LOG_WARN("result is null", K(ret));
                    } else {
                      LOG_INFO("Inner select from oceanbase.__all_help_topic join oceanbase.__all_help_relation",
                          K(sql_result->result_set()));
                      if (OB_SUCC(sql_result->next())) {};
                      ROW_NUM(sql_result, topic_num);
                      if (topic_num == 0) {
                        // do nothing
                      } else if (topic_num == 1) {
                        /* topic: name, descrition and example  */
                        /* 这一步可以优化联合查询,通过上一次联合查询得到对应的help_topic_id, 下同
                         * help JSON_CONTAINS_PATH(tested) */
                        if (OB_FAIL(sql.assign_fmt(
                                "select name, description, example from oceanbase.__all_help_topic"
                                " join oceanbase.__all_help_relation on oceanbase.__all_help_topic.help_topic_id ="
                                " oceanbase.__all_help_relation.help_topic_id"
                                " where oceanbase.__all_help_relation.help_keyword_id = %ld",
                                help_keyword_id))) {
                          LOG_WARN("append sql failed", K(ret), K(sql));
                        } else if (OB_FAIL(sql_gen.gen_select_str(sql.ptr()))) {
                          LOG_WARN("fail to generate select string", K(ret));
                        } else {
                          sql_gen.assign_sql_str(select_sql);
                        }
                      } else {
                        /* help topic and help category with keyword, 优化方法同上
                         * help CONSTRAINT_SCHEMA */
                        if (OB_FAIL(sql.assign_fmt(
                                "select name, \"N\" as is_it_category from oceanbase.__all_help_topic"
                                " join oceanbase.__all_help_relation on oceanbase.__all_help_topic.help_topic_id ="
                                " oceanbase.__all_help_relation.help_topic_id"
                                " where oceanbase.__all_help_relation.help_keyword_id = %ld"
                                " union all"
                                " select name, \"Y\" as is_it_category from oceanbase.__all_help_category where name = "
                                "\"%.*s\"",
                                help_keyword_id,
                                len,
                                mask))) {
                          LOG_WARN("append sql failed", K(ret), K(sql));
                        } else if (OB_FAIL(sql_gen.gen_select_str(sql.ptr()))) {
                          LOG_WARN("fail to generate select string", K(ret));
                        } else {
                          sql_gen.assign_sql_str(select_sql);
                        }
                      }
                    }
                  }
                }
              }
            }
          } else if (topic_num == 1) {
            /* topic: name, descrition and example
             * help HELP_DATE (tested) */
            if (OB_FAIL(sql_gen.gen_select_str(sql.ptr()))) {
              LOG_WARN("fail to generate select string", K(ret));
            } else {
              sql_gen.assign_sql_str(select_sql);
            }
          } else {
            ret = OB_ERR_UNEXPECTED;
            LOG_WARN("[gq] something error in oceanbase.__all_help_table");
          }
        }
        /* search oceanbase.__all_help_category */
        if (topic_num == 0) {
          if (OB_FAIL(sql.assign_fmt(
                  "select help_category_id, name from oceanbase.__all_help_category where name = \"%.*s\"",
                  len,
                  mask))) {
            LOG_WARN("append sql failed", K(ret), K(sql));
          } else if (OB_FAIL(
                         conn->execute_read(session_info_->get_effective_tenant_id(), sql.ptr(), res, true, false))) {
            LOG_WARN("execute sql failed", K(ret), K(sql));
          } else {
            if (OB_ISNULL(sql_result = dynamic_cast<observer::ObInnerSQLResult*>(res.get_result()))) {
              LOG_WARN("result is null", K(ret));
            } else {
              LOG_INFO("Inner select from oceanbase.__all_help_category", K(sql_result->result_set()));
              if (OB_SUCC(sql_result->next())) {
                sql_result->get_nchar("name", help_category_name);
                sql_result->get_int("help_category_id", help_category_id);
              }
              ROW_NUM(sql_result, category_num);
              if (category_num == 0) {
                // do_nothing
                if (OB_FAIL(
                        sql.assign_fmt("select * from oceanbase.__all_help_topic where help_category_id = \"-1\""))) {
                  LOG_WARN("append sql failed", K(ret), K(sql));
                } else if (OB_FAIL(sql_gen.gen_select_str(sql.ptr()))) {
                  LOG_WARN("fail to generate select string", K(ret));
                } else {
                  sql_gen.assign_sql_str(select_sql);
                }
              } else if (category_num == 1) {
                /* HELP Geographic Features (tested) */
                if (OB_FAIL(sql.assign_fmt(" select \"%.*s\" as source_category_name, name, \"N\" as is_it_category "
                                           "from oceanbase.__all_help_topic where help_category_id = %ld"
                                           " union all"
                                           " select \"%.*s\" as source_category_name, name, \"Y\" as is_it_category "
                                           "from oceanbase.__all_help_category where parent_category_id = %ld",
                        len,
                        mask,
                        help_category_id,
                        len,
                        mask,
                        help_category_id))) {
                  LOG_WARN("append sql failed", K(ret), K(sql));
                } else if (OB_FAIL(sql_gen.gen_select_str(sql.ptr()))) {
                  LOG_WARN("fail to generate select string", K(ret));
                } else {
                  sql_gen.assign_sql_str(select_sql);
                }
              } else {
                // mysql does something, 但我觉得应该不能到达这里
                ret = OB_ERR_UNEXPECTED;
                LOG_ERROR("oceanbase.__all_help_catogory has some rows having the same name");
              }
            }
          }
        }
      }
    }
  } else {
    LOG_WARN("[gq] ObHelpResoler err");
    ret = OB_ERR_UNEXPECTED;
  }
  LOG_INFO("show select sql", K(ret), K(select_sql));
  if (OB_FAIL(parse_and_resolve_select_sql(select_sql))) {
    LOG_WARN("fail to parse and resolve select sql", K(ret), K(select_sql));
  }

  // if (OB_SUCC(ret) && synonym_checker.has_synonym()) {
  // 	if (OB_FAIL(add_synonym_obj_id(synonym_checker, false /* error_with_exist */))) {
  // 	LOG_WARN("add_synonym_obj_id failed", K(ret), K(synonym_checker.get_synonym_ids()));
  // 	}
  // }

  if (OB_LIKELY(OB_SUCCESS == ret && NULL != stmt_)) {
    if (OB_UNLIKELY(stmt::T_SELECT != stmt_->get_stmt_type())) {
      ret = OB_ERR_UNEXPECTED;
      LOG_WARN("unexpected stmt type", K(stmt_->get_stmt_type()));
    } else {
      ObSelectStmt* select_stmt = static_cast<ObSelectStmt*>(stmt_);
      // select_stmt->set_is_from_show_stmt(true);
      select_stmt->set_select_type(NOT_AFFECT_FOUND_ROWS);
      // select_stmt->set_literal_stmt_type(show_resv_ctx.stmt_type_);
      select_stmt->set_select_type(NOT_AFFECT_FOUND_ROWS);
      if (OB_FAIL(select_stmt->formalize_stmt(session_info_))) {
        LOG_WARN("pull select stmt all expr relation ids failed", K(ret));
      }
    }
  }
  return ret;
}

int ObHelpResolver::ObSqlStrGenerator::init(common::ObIAllocator* allocator)
{
  int ret = OB_SUCCESS;
  if (OB_UNLIKELY(NULL == (sql_buf_ = static_cast<char*>(allocator->alloc(OB_MAX_SQL_LENGTH))))) {
    ret = OB_ALLOCATE_MEMORY_FAILED;
    LOG_ERROR("failed to alloc sql buf", K(ret), K(OB_MAX_SQL_LENGTH));
  }
  return ret;
}

int ObHelpResolver::ObSqlStrGenerator::gen_select_str(const char* select_str, ...)
{
  int ret = OB_SUCCESS;
  if (OB_ISNULL(sql_buf_)) {
    ret = OB_NOT_INIT;
    LOG_WARN("sql buffer is not init", K(ret));
  } else {
    if (NULL == select_str) {
      if (OB_FAIL(databuff_printf(sql_buf_, OB_MAX_SQL_LENGTH, sql_buf_pos_, "SELECT * "))) {
        LOG_WARN("fail to add select sql string", K(ret));
      }
    } else {
      va_list select_args;
      va_start(select_args, select_str);
      if (OB_FAIL(databuff_vprintf(sql_buf_, OB_MAX_SQL_LENGTH, sql_buf_pos_, select_str, select_args))) {
        LOG_WARN("fail to add select sql string", K(ret));
      }
      va_end(select_args);
    }
  }
  return ret;
}

int ObHelpResolver::ObSqlStrGenerator::gen_limit_str(int64_t offset, int64_t row_cnt)
{
  int ret = OB_SUCCESS;
  if (OB_FAIL(databuff_printf(sql_buf_, OB_MAX_SQL_LENGTH, sql_buf_pos_, " LIMIT %ld, %ld ", offset, row_cnt))) {
    LOG_WARN("fail to gen limit string", K(ret));
  }
  return ret;
}

void ObHelpResolver::ObSqlStrGenerator::assign_sql_str(ObString& sql_str)
{
  sql_str.assign_ptr(sql_buf_, static_cast<uint32_t>(sql_buf_pos_));
}

}  // namespace sql
}  // namespace oceanbase