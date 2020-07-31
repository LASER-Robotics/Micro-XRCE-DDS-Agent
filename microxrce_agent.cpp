// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <uxr/agent/config.hpp>

#ifdef UAGENT_CLI_PROFILE
#include <uxr/agent/utils/CLI.hpp>
#else
#include <uxr/agent/utils/ArgumentParser.hpp>
#endif

#include <csignal>


int main(int argc, char** argv)
{
#ifndef _WIN32
    sigset_t signals;
    sigemptyset(&signals);
    if(sigaddset(&signals, SIGINT) && sigaddset(&signals, SIGTERM))
    {
        std::cerr << "Wrong signalset" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    sigprocmask( SIG_BLOCK, &signals, nullptr );
#endif // _WIN32

#ifdef UAGENT_CLI_PROFILE
    /* CLI application. */
    CLI::App app("eProsima Micro XRCE-DDS Agent");
    app.require_subcommand(1, 1);
    app.get_formatter()->column_width(42);

    /* CLI subcommands. */
    eprosima::uxr::cli::UDPv4Subcommand udpv4_subcommand(app);
    eprosima::uxr::cli::UDPv6Subcommand udpv6_subcommand(app);
    eprosima::uxr::cli::TCPv4Subcommand tcpv4_subcommand(app);
    eprosima::uxr::cli::TCPv6Subcommand tcpv6_subcommand(app);
#ifndef _WIN32
    eprosima::uxr::cli::TermiosSubcommand termios_subcommand(app);
    eprosima::uxr::cli::PseudoTerminalSubcommand pseudo_serial_subcommand(app);
#endif // _WIN32
    eprosima::uxr::cli::ExitSubcommand exit_subcommand(app);

    /* CLI parse. */
    std::string cli_input{};
    for (int i = 1; i < argc; ++i)
    {
        cli_input.append(argv[i]);
        cli_input.append(" ");
    }

    while (true)
    {
        try
        {
            app.parse(cli_input);
            break;
        }
        catch (const CLI::ParseError& e)
        {
            app.exit(e);
            std::cin.clear();
            std::cout << std::endl;
            std::cout << "Enter command: ";
            std::getline(std::cin, cli_input);
        }
    }

#ifdef  _WIN32
    /* Waiting until exit. */
    std::cin.clear();
    char exit_flag = 0;
    while ('q' != exit_flag)
    {
        std::cin >> exit_flag;
    }
#else
    /* Wait for SIGTERM/SIGINT instead, as reading from stdin may be redirected to /dev/null. */
    int n_signal = 0;
    sigwait(&signals, &n_signal);
#endif // _WIN32
    return 0;

#else
    /* Use built-in argument parser */
    if (2 > argc)
    {
        eprosima::uxr::agent::parser::utils::usage();
        return 1;
    }
    std::string chosen_transport(argv[1]);
    auto valid_transport = eprosima::uxr::agent::parser::utils::check_transport(chosen_transport);
    std::thread agent_thread;
    if (eprosima::uxr::agent::TransportKind::INVALID != valid_transport)
    {
        switch (valid_transport)
        {
            case eprosima::uxr::agent::TransportKind::UDP4:
            {
                agent_thread = std::move(eprosima::uxr::agent::create_agent_thread<
                    eprosima::uxr::UDPv4Agent>(argc, argv, valid_transport, &signals));
                break;
            }
            case eprosima::uxr::agent::TransportKind::UDP6:
            {
                agent_thread = std::move(eprosima::uxr::agent::create_agent_thread<
                    eprosima::uxr::UDPv6Agent>(argc, argv, valid_transport, &signals));
                break;
            }
            case eprosima::uxr::agent::TransportKind::TCP4:
            {
                agent_thread = std::move(eprosima::uxr::agent::create_agent_thread<
                    eprosima::uxr::TCPv4Agent>(argc, argv, valid_transport, &signals));
                break;
            }
            case eprosima::uxr::agent::TransportKind::TCP6:
            {
                agent_thread = std::move(eprosima::uxr::agent::create_agent_thread<
                    eprosima::uxr::TCPv6Agent>(argc, argv, valid_transport, &signals));
                break;
            }
#ifndef _WIN32
            case eprosima::uxr::agent::TransportKind::SERIAL:
            {
                agent_thread = std::move(eprosima::uxr::agent::create_agent_thread<
                    eprosima::uxr::TermiosAgent>(argc, argv, valid_transport, &signals));
                break;
            }
            case eprosima::uxr::agent::TransportKind::PSEUDOTERMINAL:
            {
                agent_thread = std::move(eprosima::uxr::agent::create_agent_thread<
                    eprosima::uxr::PseudoTerminalAgent>(argc, argv, valid_transport, &signals));
                break;
            }
        }
        agent_thread.join();
        return 0;
    }
    else
    {
        std::cerr << "Error: chosen transport '" << chosen_transport << "' is invalid!" << std::endl;
        eprosima::uxr::agent::parser::utils::usage();
        return 1;
    }

#endif // _WIN32
#endif // UAGENT_CLI_PROFILE
}
