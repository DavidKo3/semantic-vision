#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>


#include "Includes.h"
#include "Utilities.h"

using grpc::Channel;
using grpc::ChannelArguments;
using grpc::ClientContext;

template <class T> static string checkStatus(Status status, T* response)
{
    if (status.ok())
        return (*response).status();
    else
        return status.error_message();
}

static bool checkKazeAkaze(string descriptor, string detector)
{
    if ((strncmp(descriptor.c_str(),"KAZE", 4) == 0) && (strncmp(detector.c_str(),"KAZE", 4) != 0))
    {
        cout << "KAZE features are compatible only with KAZE keypoints. Abort" << endl;
        return false;
    }
    if ((strncmp(descriptor.c_str(),"AKAZE", 5) == 0) && (strncmp(detector.c_str(),"AKAZE", 5) != 0))
    {
        cout << "AKAZE features are compatible only with AKAZE keypoints. Abort" << endl;
        return false;
    }
    return true;
}



class MatchingAPIClient{
public:
    MatchingAPIClient(std::shared_ptr<Channel> channel)
    : stub_(MatchApi::NewStub(channel)) {}

    string getKP(string image, string detector, string parameters, keypointResponse* response)
    {
        keypointRequest request;
        request.set_image(image);
        request.set_parameters(parameters);
        request.set_detector_name(detector);
        ClientContext context;

        Status status = stub_->getKP(&context, request, response);
        google::protobuf::RepeatedPtrField<::MatchingApi::keyPoint > kps = (*response).keypoints();
        return checkStatus(status, response);
    }

    string getDesc(string image, string descriptor, string descparams, string detector, string detparams, descriptorResponse* response)
    {
        if (!checkKazeAkaze(descriptor, detector))
            return "Error";
        descriptorRequest request;
        request.set_detector_name(detector);
        request.set_image(image);
        request.set_descriptor_name(descriptor);
        request.set_desc_parameters(descparams);
        request.set_det_parameters(detparams);
        ClientContext context;

        Status status = stub_->getDescByImage(&context, request, response);
        return checkStatus(status, response);
    }

    string getDescByKp(string image, string descriptor, string descparams, keypointResponse &inputKeypoints,
            descriptorResponse* response)
    {
        cout << "Please ensure that if you want KAZE/AKAZE descriptor then you need to send KAZE/AKAZE "
                "keypoints as input or you'll get an error" << endl;

        descriptorByKpsRequest request;
        request.set_descriptor_name(descriptor);
        request.set_desc_parameters(descparams);
        request.set_image(image);
        ClientContext context;
        for (auto& kp : inputKeypoints.keypoints())
        {
            MatchingApi::keyPoint* buf = request.add_keypoints();
            fillMsgKpsUsingMsgKps(kp, buf);
        }
        Status status = stub_->getDescByKps(&context, request, response);
        inputKeypoints.clear_keypoints();
        for (auto& kp : response->keypoints())
        {
            MatchingApi::keyPoint* buf = inputKeypoints.add_keypoints();
            fillMsgKpsUsingMsgKps(kp, buf);
        }
        return checkStatus(status, response);
    }

    string getMatch(descriptorResponse inputFeatures_first, descriptorResponse inputFeatures_second, matchingResponse* mresponse)
    {
        matchingRequest request;
        ClientContext context;
        if (inputFeatures_first.features_size() == 0)
        {
            return "No features given for the first image";
        }
        if (inputFeatures_second.features_size() == 0)
        {
            return "No features given for the first image";
        }
        if ((*inputFeatures_first.features().data())->onedescu().size() > 0)
        {
            for (auto& oneFeature : inputFeatures_first.features())
            {
                MatchingApi::oneDescriptor* buf = request.add_features_first();
                fillDescU(&oneFeature, buf);
            }
        }
        else if ((*inputFeatures_first.features().data())->onedescf().size() > 0)
        {
            for (auto& oneFeature : inputFeatures_first.features())
            {
                MatchingApi::oneDescriptor* buf = request.add_features_first();
                fillDescF(&oneFeature, buf);
            }
        }

        if ((*inputFeatures_second.features().data())->onedescu().size() > 0)
        {
            for (auto& oneFeature : inputFeatures_second.features())
            {
                MatchingApi::oneDescriptor* buf = request.add_features_second();
                fillDescU(&oneFeature, buf);
            }
        }
        else if ((*inputFeatures_second.features().data())->onedescf().size() > 0)
        {
            for (auto& oneFeature : inputFeatures_second.features())
            {
                MatchingApi::oneDescriptor* buf = request.add_features_second();
                fillDescF(&oneFeature, buf);
            }
        }

        Status status = stub_->getMatch(&context, request, mresponse);
        return checkStatus(status, mresponse);
    }


    string getMatchByImage(string image1, string image2, string detector, string det_params, string descriptor,
            string desc_params, matchingByImageResponse* response)
    {
        matchingByImageRequest request;
        ClientContext context;
        request.set_image_first(image1);
        request.set_image_second(image2);
        request.set_detector_name(detector);
        request.set_det_parameters(det_params);
        request.set_descriptor_name(descriptor);
        request.set_desc_parameters(desc_params);
        Status status = stub_->getMatchByImage(&context, request, response);
        return checkStatus(status, response);
    }

    string getTransformParameters(string transform_type, string transform_input_parameters,
            ::google::protobuf::RepeatedPtrField< ::MatchingApi::keyPoint > first_kps,
            ::google::protobuf::RepeatedPtrField< ::MatchingApi::keyPoint > second_kps,
            ::google::protobuf::RepeatedPtrField< ::MatchingApi::matchedPoint > matches,
            transformResponse* reply)
    {
        transformRequest request;
        ClientContext context;
        request.set_transform_input_parameters(transform_input_parameters);
        request.set_transform_type(transform_type);
        for (auto& kp : first_kps)
        {
            MatchingApi::keyPoint* buf = request.add_keypoints_first();
            fillMsgKpsUsingMsgKps(kp, buf);
        }
        for (auto& kp : second_kps)
        {
            MatchingApi::keyPoint* buf = request.add_keypoints_second();
            fillMsgKpsUsingMsgKps(kp, buf);
        }
        if (matches.size() == 0)
            return "No matches given";
        for (auto& oneMatch : matches)
        {
            auto buf = request.add_all_matches();
            buf->set_distance(oneMatch.distance());
            buf->set_imgidx(oneMatch.imgidx());
            buf->set_queryidx(oneMatch.queryidx());
            buf->set_trainidx(oneMatch.trainidx());
        }
        Status status = stub_->getTransformParameters(&context, request, reply);
        return checkStatus(status, reply);
    }

    string getTransformParametersByImage(string image1, string image2, string detector, string det_params, string descriptor,
            string desc_params, string transform_type, string transform_input_parameters, transformResponse* reply)
    {
        transformByImageRequest request;
        ClientContext context;
        request.set_desc_parameters(desc_params);
        request.set_descriptor_name(descriptor);
        request.set_det_parameters(det_params);
        request.set_detector_name(detector);
        request.set_transform_input_parameters(transform_input_parameters);
        request.set_transform_type(transform_type);
        request.set_image_first(image1);
        request.set_image_second(image2);

        Status status = stub_->getTransformParametersByImage(&context, request, reply);
        return checkStatus(status, reply);
    }

    string getClosestImages(string q_image, vector<string> image_base, string detector, string det_params, string descriptor,
                                         string desc_params, int numImagesToRetrieve, int numOfClusters, imageRetrievalResponse* reply)
    {
        imageRetrievalRequest request;
        ClientContext context;
        request.set_desc_parameters(desc_params);
        request.set_descriptor_name(descriptor);
        request.set_det_parameters(det_params);
        request.set_detector_name(detector);
        request.set_input_image(q_image);

        for (auto& oneImage : image_base)
        {
            string * buf = request.add_image_base();
            (*buf) = oneImage;
        }
        request.set_numofimagestoretrieve(numImagesToRetrieve);
        request.set_numofclusters(numOfClusters);
        Status status = stub_->getClosestImages(&context, request, reply);
        return checkStatus(status, reply);
    }
private:
    std::unique_ptr<MatchApi::Stub> stub_;
};


int main()
{
    grpc::ChannelArguments ch_args;
    ch_args.SetMaxReceiveMessageSize(-1);
    MatchingAPIClient client(grpc::CreateCustomChannel("localhost:50051", grpc::InsecureChannelCredentials(), ch_args));
    string image("../Woods_shiftrot_120.jpg");
    string image2("../Woods.jpg");

    string descriptor("AKAZE");
    string detector("AKAZE");
    string detector_params("");
    string desc_params("");
    string transf_type("ShiftRot");
    string transf_params_in("");
    string reply;
    string image_bytes = getImageString(image);
    string image_bytes2 = getImageString(image2);


    //getKP usage
    {
        keypointResponse responsekp;
        reply = client.getKP(image_bytes, detector, detector_params, &responsekp);
        vector<KeyPoint> keypointsFromMAPI;
        for (auto& oneKP: responsekp.keypoints())
        {
            KeyPoint kp;
            kp.angle=oneKP.angle();
            kp.class_id=oneKP.class_id();
            kp.octave=oneKP.octave();
            kp.pt.x=oneKP.x();
            kp.pt.y=oneKP.y();
            kp.response=oneKP.response();
            kp.size=oneKP.size();
            keypointsFromMAPI.push_back(kp);
        }
        string kpImage = responsekp.uiimage();
        string decoded = base64_decode(kpImage);
        Mat checkKp = getMat(decoded);
        imwrite("check_kp.png", checkKp);
        cout << "get keypoints " << reply << endl;
    }

    //getDescByImg usage
    {
        descriptorResponse response;
        reply = client.getDesc(image_bytes, descriptor, desc_params, detector, detector_params, &response);
        cout << "get descriptor by image " << reply << " " << response.status() << endl;
    }

    //getDescByKps usage
    {
        keypointResponse responsekp;
        reply = client.getKP(image_bytes, detector, detector_params, &responsekp);
        cout << "get keypoints " << reply << endl;
        descriptorResponse response_bykps;
        reply = client.getDescByKp(image_bytes, descriptor, desc_params, responsekp, &response_bykps);
        cout << "get descriptor by keypoints " << reply << endl;
    }


    //getMatch usage
    {
        descriptorResponse responseDesc1, responseDesc2;
        reply = client.getDesc(image_bytes, descriptor, desc_params, detector, detector_params, &responseDesc1);
        cout << "get descriptor by image " << reply << endl;
        reply = client.getDesc(image_bytes2, descriptor, desc_params, detector, detector_params, &responseDesc2);
        cout << "get descriptor by image " << reply << endl;
        matchingResponse mresponse;
        reply = client.getMatch(responseDesc1, responseDesc2, &mresponse);
        cout << "get match using computed descriptors " << reply << endl;
    }

    //getMatchByImg usage
    {
        matchingByImageResponse mresponse;
        reply = client.getMatchByImage(image_bytes, image_bytes2, detector, detector_params, descriptor,
                                       desc_params, &mresponse);
        string mImage = mresponse.uiimage();
        string decoded_M = base64_decode(mImage);
        Mat checkM = getMat(decoded_M);
        imwrite("check_m.png", checkM);
        cout << "get match by images " << reply << endl;
    }

    //getTransformParameters usage
    {
        keypointResponse responsekp1, responsekp2;
        reply = client.getKP(image_bytes, detector, detector_params, &responsekp1);
        reply = client.getKP(image_bytes2, detector, detector_params, &responsekp2);
        descriptorResponse responsedesc1, responsedesc2;
        reply = client.getDescByKp(image_bytes, descriptor, desc_params, responsekp1, &responsedesc1);
        reply = client.getDescByKp(image_bytes2, descriptor, desc_params, responsekp2, &responsedesc2);

        matchingResponse mresponse;

        reply = client.getMatch(responsedesc1, responsedesc2, &mresponse);

        transformResponse responseTransform;
        reply = client.getTransformParameters(transf_type, transf_params_in, responsekp1.keypoints(),
                                              responsekp2.keypoints(),
                                              mresponse.all_matches(), &responseTransform);

        cout << "get transform by matched keypoints " << reply << endl;
        cout << "Transform parameters are: " << endl;
        for (auto &oneParam : responseTransform.transform_parameters())
            cout << oneParam << " ";
        cout << endl;
    }

    //getTransformByImage usage
    {
        transformResponse responseTransform;
        reply = client.getTransformParametersByImage(image_bytes, image_bytes2, detector, detector_params,
                                                     descriptor, desc_params,
                                                     transf_type, transf_params_in, &responseTransform);


        cout << "get transform by images " << reply << endl;
        cout << "Transform parameters are: " << endl;
        string rImage = responseTransform.resultimage();
        string decoded_R = base64_decode(rImage);
        Mat checkR = getMat(decoded_R);
        imwrite("check_r.png", checkR);

        string mixedImage = responseTransform.uiimage();
        string decoded_mixed = base64_decode(mixedImage);
        Mat checkMixed = getMat(decoded_mixed);
        imwrite("check_mixed.png", checkMixed);

        for (auto &oneParam : responseTransform.transform_parameters())
            cout << oneParam << " ";
        cout << endl;
    }

    //getClosestImages usage
    {
        imageRetrievalResponse retrievalResponse;
        vector<string> database;
        struct dirent *entry = nullptr;
        DIR *dp = nullptr;
        dp = opendir("../../PizzaDatabase/Train"); //set your own path to database of your chose
        if (dp != nullptr)
        {
            while ((entry = readdir(dp)))
            {
                char path[100];
                strcpy(path, "../../PizzaDatabase/Train/");
                strcat(path, entry->d_name);
                FILE *in_file = fopen(path, "rb");

                fseek(in_file, 0L, SEEK_END);
                int sz = ftell(in_file);
                if (sz < 0)
                    continue;
                database.emplace_back(getImageString(path));
                fclose(in_file);
            }
        }
        closedir(dp);

        vector<string> q_database;
        struct dirent *qentry = nullptr;
        DIR *qdp = nullptr;
        qdp = opendir("../../PizzaDatabase/Query");
        if (qdp != nullptr)
        {
            while ((qentry = readdir(qdp)))
            {
                char path[100];
                strcpy(path, "../../PizzaDatabase/Query/");
                strcat(path, qentry->d_name);
                FILE *in_file = fopen(path, "rb");

                fseek(in_file, 0L, SEEK_END);
                int sz = ftell(in_file);
                if (sz < 0)
                    continue;
                q_database.emplace_back(getImageString(path));
                fclose(in_file);
            }
        }
        closedir(qdp);
        cout << "detector " << detector << " descriptor " << descriptor << endl;
        string q_image = getImageString("../../PizzaDatabase/Query/pizza-2780415_960_720.jpg");
        reply = client.getClosestImages(q_image, database, detector, detector_params,
                                        descriptor, desc_params,
                                        10, 1000, &retrievalResponse);
        if (retrievalResponse.distances_size() != 0)
        {
            Mat concatted, concattedPre;
            Mat original = getMat(q_image);
            concattedPre = original.clone();
            int width = 640;
            int height = 480;
            resize(concattedPre, concatted, Size(width, height), 0, 0, INTER_CUBIC);
            string distance = "Distance ";
            for (auto &oneImage : retrievalResponse.images())
            {
                Mat buf = getMat(oneImage);
                Mat buf2;
                resize(buf, buf2, Size(width, height), 0, 0, INTER_CUBIC);
                hconcat(concatted, buf2, concatted);
            }
            std::string out_string;
            std::stringstream ss;
            out_string = ss.str();
            imwrite("IR_result.png", concatted);
        }
        string concatImage = retrievalResponse.uiimage();
        string decoded_concat = base64_decode(concatImage);
        Mat checkConcat = getMat(decoded_concat);
        imwrite("check_concat.png", checkConcat);
        cout << "get closest image " << reply << endl;
        cout << endl;
    }
    return 0;
}


