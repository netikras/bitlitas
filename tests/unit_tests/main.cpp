// Copyright (c) 2018, Bitlitas
// All rights reserved. Based on Cryptonote and Monero.

#include "gtest/gtest.h"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/program_options.hpp>

#include "p2p/net_node.h"
#include "p2p/net_node.inl"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.inl"
#include "include_base_utils.h"
#include "string_tools.h"
#include "common/command_line.h"
#include "common/util.h"
#include "unit_tests_utils.h"

namespace po = boost::program_options;

boost::filesystem::path unit_test::data_dir;

namespace nodetool { template class node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core>>; }
namespace cryptonote { template class t_cryptonote_protocol_handler<cryptonote::core>; }

int main(int argc, char** argv)
{
  tools::on_startup();
  epee::string_tools::set_module_name_and_folder(argv[0]);
  mlog_configure(mlog_get_default_log_path("unit_tests.log"), true);
  epee::debug::get_set_enable_assert(true, false);

  ::testing::InitGoogleTest(&argc, argv);

  po::options_description desc_options("Command line options");
  const command_line::arg_descriptor<std::string> arg_data_dir = { "data-dir", "Data files directory" };
  command_line::add_arg(desc_options, arg_data_dir, "");

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), vm);
    po::notify(vm);
    return true;
  });
  if (! r)
    return 1;

  if (command_line::is_arg_defaulted(vm, arg_data_dir))
    unit_test::data_dir = boost::filesystem::canonical(boost::filesystem::path(epee::string_tools::get_current_module_folder()))
                          .parent_path().parent_path().parent_path().parent_path()
                          .append("tests").append("data");
  else
    unit_test::data_dir = command_line::get_arg(vm, arg_data_dir);

  return RUN_ALL_TESTS();
}
