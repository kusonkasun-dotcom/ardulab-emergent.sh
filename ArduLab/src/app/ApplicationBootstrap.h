#pragma once

// ApplicationBootstrap — composition root (Plan §4.1).
//
// Owns the lifetime of application-level services and the main window and
// injects interfaces into their consumers. Startup errors are reported here,
// never handled inside MainWindow.
//
// Foundation phase: the catalog is the in-memory IComponentCatalog adapter
// seeded from the bundled canonical example. The SQLite adapter replaces it
// in the database phase without touching MainWindow or ComponentManager.

#include <memory>

namespace ardulab::app {

class ApplicationBootstrap final
{
public:
    ApplicationBootstrap();
    ~ApplicationBootstrap();

    ApplicationBootstrap(const ApplicationBootstrap&) = delete;
    ApplicationBootstrap& operator=(const ApplicationBootstrap&) = delete;

    /// Compose services and show the main window.
    /// Returns 0 on success, or a non-zero process exit code on fatal startup failure.
    int start();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace ardulab::app
