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

#include "observer/virtual_table/ob_mysql_help_category_table.h"

#include "share/schema/ob_schema_struct.h"
#include "share/schema/ob_schema_getter_guard.h"
#include "share/schema/ob_schema_printer.h"
#include "common/sql_mode/ob_sql_mode_utils.h"
#include "sql/session/ob_sql_session_info.h"

using namespace oceanbase::common;
using namespace oceanbase::share::schema;
namespace oceanbase {
namespace observer {

ObMySQLHelpCategoryTable::ObMySQLHelpCategoryTable() : ObVirtualTableScannerIterator(), tenant_id_(OB_INVALID_ID)
{}

ObMySQLHelpCategoryTable::~ObMySQLHelpCategoryTable()
{}

void ObMySQLHelpCategoryTable::reset()
{
  tenant_id_ = OB_INVALID_ID;
  ObVirtualTableScannerIterator::reset();
}

int ObMySQLHelpCategoryTable::inner_get_next_row(common::ObNewRow*& row)
{
  int ret = OB_SUCCESS;
  if (OB_ISNULL(allocator_) || OB_ISNULL(schema_guard_) || OB_ISNULL(session_)) {
    ret = OB_NOT_INIT;
    SERVER_LOG(WARN, "argument is NULL", K(allocator_), K(schema_guard_), K(session_), K(ret));
  } else if (OB_UNLIKELY(OB_INVALID_ID == tenant_id_)) {
    ret = OB_NOT_INIT;
    SERVER_LOG(WARN, "tenant_id is invalid", K(ret));
  } else {
    if (!start_to_read_) {
      ObObj* cells = NULL;
      if (OB_ISNULL(cells = cur_row_.cells_)) {
        ret = OB_ERR_UNEXPECTED;
        SERVER_LOG(ERROR, "cur row cell is NULL", K(ret));
      } else { 
      }
      if (OB_SUCC(ret)) {
        scanner_it_ = scanner_.begin();
        start_to_read_ = true;
      }
    }
    if (OB_SUCC(ret)) {
      if (start_to_read_) {
        if (OB_SUCCESS != (ret = scanner_it_.get_next_row(cur_row_))) {
          if (OB_ITER_END != ret) {
            SERVER_LOG(WARN, "fail to get next row", K(ret));
          }
        } else {
          row = &cur_row_;
        }
      } else {
        // do nothing
      }
    }
  }
  return ret;
}

}  // namespace observer
}  // namespace oceanbase
