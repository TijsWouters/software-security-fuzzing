#include <iostream>
#include <string>
#include "lib3mf_implicit.hpp"
using namespace Lib3MF;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "usage: " << argv[0] << " <path-to-3mf>\n";
        return 2;
    }
    const std::string path = argv[1];

    try
    {
        auto wrapper = Lib3MF::CWrapper::loadLibrary();

        Lib3MF_uint32 major = 0, minor = 0, micro = 0;
        wrapper->GetLibraryVersion(major, minor, micro);
        std::cout << "Lib3MF version: " << major << "." << minor << "." << micro << "\n";

        Lib3MF::PModel model = wrapper->CreateModel();
        if (!model)
        {
            std::cerr << "CreateModel returned null.\n";
            return 2;
        }

        // Create a 3MF reader and parse the file.
        Lib3MF::PReader reader = model->QueryReader("3mf"); // creates a reader for a specific file type
        reader->SetStrictModeActive(false);                 // be permissive so fuzz data goes deeper
        reader->ReadFromFile(path);                         // read model from file

        // Print warnings (if any)
        Lib3MF_uint32 warnCount = reader->GetWarningCount();
        for (Lib3MF_uint32 i = 0; i < warnCount; ++i)
        {
            Lib3MF_uint32 code = 0;
            std::string msg = reader->GetWarning(i, code);
            std::cout << "[warning] (" << code << ") " << msg << "\n";
        }

        // --- 1) Mesh objects: poke Beam Lattice options (clip/representation/balls)
        {
            auto it = model->GetMeshObjects(); // typed iterator
            while (it->MoveNext())
            {
                auto mesh = it->GetCurrentMeshObject();
                (void)mesh->IsManifoldAndOriented(); // geometry sanity check

                auto bl = mesh->BeamLattice(); // view on beamlattice
                eBeamLatticeClipMode clipMode{};
                Lib3MF_uint32 clipMeshID{};
                bl->GetClipping(clipMode, clipMeshID); // query clipping
                Lib3MF_uint32 repMeshID{};
                (void)bl->GetRepresentation(repMeshID); // has representation?
                eBeamLatticeBallMode ballMode{};
                Lib3MF_double ballR{};
                bl->GetBallOptions(ballMode, ballR); // ball options
            }
        }

        // --- 2) Slice stacks: touch slices and polygon rings
        {
            auto ss = model->GetSliceStacks();
            while (ss->MoveNext())
            {
                auto stack = ss->GetCurrentSliceStack();
                auto nSlices = stack->GetSliceCount();
                if (nSlices > 0)
                {
                    auto slice0 = stack->GetSlice(0);
                    auto nPolys = slice0->GetPolygonCount();
                    if (nPolys > 0)
                    {
                        std::vector<Lib3MF_uint32> ring;
                        slice0->GetPolygonIndices(0, ring); // read first polygon loop
                    }
                }
                (void)stack->GetOwnPath(); // exercise package path
            }
        }

        // --- 3) Components graph: traverse and read transforms
        {
            auto compIt = model->GetComponentsObjects();
            while (compIt->MoveNext())
            {
                auto compObj = compIt->GetCurrentComponentsObject();
                auto count = compObj->GetComponentCount();
                for (Lib3MF_uint32 i = 0; i < count; ++i)
                {
                    auto comp = compObj->GetComponent(i);
                    auto t = comp->GetTransform();           // read transform
                    auto refObj = comp->GetObjectResource(); // follow reference
                    (void)t;
                    (void)refObj;
                }
            }
        }

        // --- 4) Keystore/encrypted parts: enumerate resource data
        {
            auto ks = model->GetKeyStore();
            if (ks)
            {
                auto n = ks->GetResourceDataCount();
                for (Lib3MF_uint64 i = 0; i < n; ++i)
                {
                    auto rd = ks->GetResourceData(i);
                    auto part = rd->GetPath(); // encrypted part path
                    auto alg = rd->GetEncryptionAlgorithm();
                    auto comp = rd->GetCompression();
                    std::vector<Lib3MF_uint8> aad;
                    rd->GetAdditionalAuthenticationData(aad); // extra auth data
                    (void)part;
                    (void)alg;
                    (void)comp;
                }
            }
        }

        // --- 5) Stress the writer: merge (expensive) and serialize to memory
        {
            auto merged = model->MergeToModel(); // force deep merge
            (void)merged->GetOutbox();           // compute bbox
            auto writer = merged->QueryWriter("3mf");
            std::vector<Lib3MF_uint8> out;
            writer->WriteToBuffer(out); // round-trip serialize
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1; // AFL++ treats crashes via signals; nonzero exit is fine for failures
    }
}
