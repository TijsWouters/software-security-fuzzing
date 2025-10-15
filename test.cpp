#include <iostream>
#include <string>
#include "lib3mf_implicit.hpp"

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <path-to-3mf>\n";
        return 2;
    }
    const std::string path = argv[1];

    try {
        auto wrapper = Lib3MF::CWrapper::loadLibrary();

        Lib3MF_uint32 major = 0, minor = 0, micro = 0;
        wrapper->GetLibraryVersion(major, minor, micro);
        std::cout << "Lib3MF version: " << major << "." << minor << "." << micro << "\n";

        auto model = wrapper->CreateModel();
        if (!model) {
            std::cerr << "CreateModel returned null.\n";
            return 2;
        }

        // Create a 3MF reader and parse the file.
        auto reader = model->QueryReader("3mf"); // creates a reader for a specific file type
        reader->SetStrictModeActive(false);      // be permissive so fuzz data goes deeper
        reader->ReadFromFile(path);              // read model from file

        // Print warnings (if any)
        Lib3MF_uint32 warnCount = reader->GetWarningCount();
        for (Lib3MF_uint32 i = 0; i < warnCount; ++i) {
            Lib3MF_uint32 code = 0;
            std::string msg = reader->GetWarning(i, code);
            std::cout << "[warning] (" << code << ") " << msg << "\n";
        }

        // Touch a few parts of the model to exercise code paths.
        auto buildItems = model->GetBuildItems();
        auto objects    = model->GetObjects();
        auto meshes     = model->GetMeshObjects();

        std::cout << "Parsed OK. build_items=" << (unsigned long long)buildItems->Count()
                  << " objects=" << (unsigned long long)objects->Count()
                  << " mesh_objects=" << (unsigned long long)meshes->Count()
                  << "\n";

        // Also call GetOutbox to force bounding-box calc.
        (void)model->GetOutbox();

        std::cout << "Done.\n";
        return 0;

    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1; // AFL++ treats crashes via signals; nonzero exit is fine for failures
    }
}
