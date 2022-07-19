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

#ifndef OCEANBASE_SRC_OBSERVER_VIRTUAL_TABLE_OB_MYSQL_HELP_CATEGORY_TABLE_H_
#define OCEANBASE_SRC_OBSERVER_VIRTUAL_TABLE_OB_MYSQL_HELP_CATEGORY_TABLE_H_

#include "share/ob_virtual_table_scanner_iterator.h"

namespace oceanbase {
namespace sql {
class ObSQLSessionInfo;
}
namespace observer {
class ObMySQLHelpCategoryTable : public common::ObVirtualTableScannerIterator {
private:
  enum MySQLHelpCategoryTableColumns {
    HELP_CATEGORY_ID = 16,
    NAME,
    PARENT_CATEGORY_ID,
    URL,
  };

public:
  ObMySQLHelpCategoryTable();
  virtual ~ObMySQLHelpCategoryTable();

  virtual int inner_get_next_row(common::ObNewRow*& row);
  virtual void reset();
  inline void set_tenant_id(const uint64_t tenant_id)
  {
    tenant_id_ = tenant_id;
  }

private:
  uint64_t tenant_id_;

private:
  DISALLOW_COPY_AND_ASSIGN(ObMySQLHelpCategoryTable);
};
}  // namespace observer
}  // namespace oceanbase

#endif /* OCEANBASE_SRC_OBSERVER_VIRTUAL_TABLE_OB_MYSQL_PROC_TABLE_H_ */
