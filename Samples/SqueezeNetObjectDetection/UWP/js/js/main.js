var _model;
var _session;
var _labels;


window.onload = function () {
    // setup the button click event handlers
    document.getElementById("loadModelButton").onclick = function (evt) {
        loadModel();
    }
    document.getElementById("pickImageButton").onclick = function (evt) {
        pickImage();
    }
    document.getElementById("resetButton").onclick = function (evt) {
        reset();
    }

    // disable our buttons that you need load to happen first
    document.getElementById("pickImageButton").disabled = true;
    document.getElementById("resetButton").disabled = true;

    // Parse labels from label json file.  We know the file's 
    // entries are already sorted in order.
    var uri = new Windows.Foundation.Uri("ms-appx:///assets/labels.json");
    Windows.Storage.StorageFile.getFileFromApplicationUriAsync(uri).then(function (file) {
        Windows.Storage.FileIO.readTextAsync(file).then(function (jsonData) { 
            _labels = JSON.parse(jsonData);
        });
    });
}


function getDeviceKind()
{
    switch (document.getElementById("deviceKind").selectedIndex) {
        case 0:
            return Windows.AI.MachineLearning.LearningModelDeviceKind.Default;
        case 1:
            return Windows.AI.MachineLearning.LearningModelDeviceKind.Cpu;
        case 2:
            return Windows.AI.MachineLearning.LearningModelDeviceKind.DirectX;
        case 3:
            return Windows.AI.MachineLearning.LearningModelDeviceKind.DirectXHighPerformance;
        case 4:
            return Windows.AI.MachineLearning.LearningModelDeviceKind.DirectXMinPower;
    }
    return Windows.AI.MachineLearning.LearningModelDeviceKind.Default;
}

function loadModel() {
    // Load and create the model 
    var uri = new Windows.Foundation.Uri("ms-appx:///assets/model.onnx")
    // get the storagefile first
    Windows.Storage.StorageFile.getFileFromApplicationUriAsync(uri).then(function (modelFile) {
        // now load the learningmodel
        Windows.AI.MachineLearning.LearningModel.loadFromStorageFileAsync(modelFile).then(function (model) {
            // store it global
            _model = model;
            // Create the session with the model and device
            var device = new Windows.AI.MachineLearning.LearningModelDevice(getDeviceKind());
            _session = new Windows.AI.MachineLearning.LearningModelSession(_model, device);
            // enable the rest of the butons
            document.getElementById("loadModelButton").disabled = true;
            document.getElementById("pickImageButton").disabled = false;
            document.getElementById("resetButton").disabled = false;
            document.getElementById("statusBlock").innerText = "Model loaded, ready to run";
        });
    });
}

function pickImage() {
    // don't let them push the button twice
    document.getElementById("pickImageButton").disabled = true;
    document.getElementById("resetButton").disabled = true;
    // Create the picker object and set options
    var openPicker = new Windows.Storage.Pickers.FileOpenPicker();
    openPicker.viewMode = Windows.Storage.Pickers.PickerViewMode.thumbnail;
    openPicker.suggestedStartLocation = Windows.Storage.Pickers.PickerLocationId.picturesLibrary;
    // Users expect to have a filtered view of their folders depending on the scenario.
    // For example, when choosing a documents folder, restrict the filetypes to documents for your application.
    openPicker.fileTypeFilter.replaceAll([".png", ".jpg", ".jpeg"]);
    // Open the picker for the user to pick a file
    openPicker.pickSingleFileAsync().then(function (file) {
        if (file) {
            // Application now has read/write access to the picked file
            file.openAsync(Windows.Storage.FileAccessMode.Read).then(function (stream) {
                // Create the decoder from the stream 
                Windows.Graphics.Imaging.BitmapDecoder.createAsync(stream).then(function (decoder) {
                    // Get the SoftwareBitmap representation of the file in BGRA8 format
                    decoder.getSoftwareBitmapAsync().then(function (softwareBitmap) {
                        // Encapsulate the image within a VideoFrame to be bound and evaluated
                        var inputFrame = Windows.Media.VideoFrame.createWithSoftwareBitmap(softwareBitmap);
                        // create a binding object from the session
                        var binding = new Windows.AI.MachineLearning.LearningModelBinding(_session);
                        // bind the input image
                        var imageTensor = Windows.AI.MachineLearning.ImageFeatureValue.createFromVideoFrame(inputFrame);
                        binding.bind("data_0", imageTensor);
                        // Process the frame with the model
                        _session.evaluateAsync(binding, null).then(function (results) {
                            // retrieve results from evaluation
                            var resultTensor = results.outputs["softmaxout_1"];
                            var resultVector = resultTensor.getAsVectorView();
                            // Find the top 3 probabilities
                            var topProbabilities = [ 0.0, 0.0, 0.0 ];
                            var topProbabilityLabelIndexes = [ 0, 0, 0 ];
                            // SqueezeNet returns a list of 1000 options, with probabilities for each, loop through all
                            for (var i = 0, len = resultVector.length; i < len; i++)
                            {
                                // is it one of the top 3?
                                for (var j = 0; j < 3; j++)
                                {
                                    if (resultVector[i] > topProbabilities[j]) {
                                        topProbabilityLabelIndexes[j] = i;
                                        topProbabilities[j] = resultVector[i];
                                        break;
                                    }
                                }
                            }
                            // Display the result
                            var message = "";
                            for (var i = 0; i < 3; i++)
                            {
                                message += "\n" + _labels[topProbabilityLabelIndexes[i]] + " with confidence of " + topProbabilities[i];
                            }
                            // preview the image stream
                            document.getElementById("previewImage").src = URL.createObjectURL(file, { oneTimeOnly: true });
                            // update the status
                            document.getElementById("statusBlock").innerText = message;
                            // let them click the buttons again
                            document.getElementById("pickImageButton").disabled = false;
                            document.getElementById("resetButton").disabled = false;
                        });
                    });
                });
            });
        } else {
            // The picker was dismissed with no selected file
        }
    });
}

function reset()
{
    // setup the buttons state, you have to load a model first
    document.getElementById("loadModelButton").disabled = false;
    document.getElementById("pickImageButton").disabled = true;
    document.getElementById("resetButton").disabled = true;
    // update the status
    document.getElementById("statusBlock").innerText = "Model unloaded";
}