#include "pch.h"

using namespace IpCamera;
using namespace MediaCaptureReader;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Platform;
using namespace std;
using namespace Windows::Foundation;
using namespace Windows::Media::Capture;

//
// Windows tests use NullMediaCapture as the real MediaCapture tries to pop up a consent UI which is nowhere to be seen
// and cannot be automatically dismissed from within the tests
//

TEST_CLASS(CameraServertTests)
{
public:

    TEST_METHOD(CX_W_CameraServer_Basic)
    {
        auto settings = ref new MediaCaptureInitializationSettings();
        settings->StreamingCaptureMode = StreamingCaptureMode::Video;

        auto capture = NullMediaCapture::Create();
        Await(capture->InitializeAsync(settings));
        
        auto server = Await(CameraServer::CreateFromMediaCaptureAsync(31416, capture));

        Assert::AreEqual(31416, server->Port);
        
        auto source = Await(HttpMjpegCaptureSource::CreateFromUriAsync("http://localhost:31416/"));

        auto reader = Await(MediaReader::CreateFromMediaSourceAsync(source->Source, AudioInitialization::Deselected, VideoInitialization::Nv12));

        for (int i = 0; i < 3; i++)
        {
            Log() << "Reading sample " << i;

            MediaReaderReadResult^ result = Await(reader->VideoStream->ReadAsync());

            Assert::IsFalse(result->Error);
            Assert::IsFalse(result->EndOfStream);

            auto sample = safe_cast<MediaSample2D^>(result->Sample);

            Assert::IsTrue(sample->Width > 0);
            Assert::IsTrue(sample->Height > 0);
        }
    }

    TEST_METHOD(CX_W_CameraServer_Misc)
    {
        auto settings = ref new MediaCaptureInitializationSettings();
        settings->StreamingCaptureMode = StreamingCaptureMode::Video;

        auto capture = NullMediaCapture::Create();
        Await(capture->InitializeAsync(settings));

        Log() << "Testing auto TCP port selection";
        auto server = Await(CameraServer::CreateFromMediaCaptureAsync(capture));
        Assert::IsTrue(server->Port > 0);

        Log() << "Testing Failed event registration";
        server->Failed += ref new TypedEventHandler<Object ^, CameraServerFailedEventArgs ^>([](Object ^, CameraServerFailedEventArgs ^e)
        {
            Log() << "Failed event raised with hr=0x" << hex << e->ErrorCode << " " << e->Message->Data();
        });

        Log() << "Testing IClosable::Close()";
        delete server;
    }
};
