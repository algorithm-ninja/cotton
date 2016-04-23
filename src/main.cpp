#include "box.hpp"
#include <vector>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

BoxCreators box_creators;
std::vector<std::pair<int, std::string>> errors;


int main(int argc, char** argv) {
    po::options_description global("Generic options");
    global.add_options()
        ("help,h", "produce help message")
        ("box-root,b", po::value<std::string>(), "specify a different folder to put sandboxes in")
        ("quiet,q", po::value<std::string>(), "less output for tty mode")
        ("json,j", po::value<std::string>(), "force JSON output");

    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("command", po::value<std::string>(), "command to execute")
        ("command-args", po::value<std::vector<std::string> >(), "arguments for command");

    po::positional_options_description pos;
    pos.add("command", 1).
        add("subargs", -1);

    po::variables_map vm;

    po::options_description generic_opts("Generic options");
    generic_opts.add(global).add(hidden);
    po::parsed_options parsed = po::command_line_parser(argc, argv).
        options(generic_opts).
        positional(pos).
        allow_unregistered().
        run();

    po::store(parsed, vm);
    vm.notify();
    if (vm.count("help") == 1) {
        std::cerr << "Usage: " << argv[0] << " [options] command [command-args...]" << std::endl;
        std::cerr << std::endl;
        std::cerr << global << std::endl;
        std::cerr << "Possible commands are:" << std::endl;
        std::cerr << " list        list available implementations" << std::endl;
        std::cerr << " check       check if a sandbox is consistent" << std::endl;
        std::cerr << " set-limit   set some limit for the program execution" << std::endl;
        std::cerr << " get-limit   read a current limit" << std::endl;
        std::cerr << " mount       make paths readable to the process" << std::endl;
        std::cerr << " umount      disable paths" << std::endl;
        std::cerr << " run         execute command" << std::endl;
        std::cerr << " get         get information on the last execution" << std::endl;
        std::cerr << " clear       cleanup the sandbox" << std::endl;
        std::cerr << " delete      delete the sandbox" << std::endl;
        return 1;
    }
    if (!vm.count("command")) {
        errors.emplace_back(1, "No command given!");
    } else {
        std::string cmd = vm["command"].as<std::string>();
        if (cmd == "list") {
        
        } else if (cmd == "create") {
        
        } else if (cmd == "check") {
        
        } else if (cmd == "set-limit") {
        
        } else if (cmd == "get-limit") {
        
        } else if (cmd == "mount") {
        
        } else if (cmd == "umount") {
        
        } else if (cmd == "run") {
        
        } else if (cmd == "get") {
        
        } else if (cmd == "clear") {
        
        } else if (cmd == "delete") {
        
        } else
            errors.emplace_back(1, "Unknown command!");
    }
}
