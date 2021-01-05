import * as fs from 'fs';
import * as path from 'path';
import * as React from 'react';
import { connect } from 'react-redux';

import { DefaultButton } from 'office-ui-fabric-react/lib/Button';
import { ChoiceGroup, IChoiceGroupOption } from 'office-ui-fabric-react/lib/ChoiceGroup';
import { MessageBar, MessageBarType } from 'office-ui-fabric-react/lib/MessageBar';
import { Spinner } from 'office-ui-fabric-react/lib/Spinner';
import { TextField } from 'office-ui-fabric-react/lib/TextField';
import Select from 'react-select';

import { winmlDataFolder } from '../../native/appData';

import Collapsible from '../../components/Collapsible';
import { setFile, setSaveFileName } from '../../datastore/actionCreators';
import IState from '../../datastore/state';
import { packagedFile } from '../../native/appData';
import { fileFromPath, showNativeOpenDialog, showNativeSaveDialog } from '../../native/dialog';
import { downloadPip, downloadPython, getLocalPython, pip, python } from '../../native/python';
import { isWeb } from '../../native/util';

import './View.css';

import log from 'electron-log';

enum Step {
    Idle,
    Downloading,
    GetPip,
    CreatingVenv,
    InstallingRequirements,
    Converting,
}

interface ISelectOpition {
    label: string;
    value: string;
  }

interface IComponentProperties {
    // Redux properties
    file: File,
    setFile: typeof setFile,
    setSaveFileName: typeof setSaveFileName,
}

interface IComponentState {
    ONNXVersion: ISelectOpition,
    console: string,
    currentStep: Step,
    error?: Error | string,
    framework: string,
    outputNames: string,
    source?: string,
}

class ConvertView extends React.Component<IComponentProperties, IComponentState> {
    private localPython?: string;

    constructor(props: IComponentProperties) {
        super(props);
        const error = isWeb() ? "The converter can't be run in the web interface" : undefined;
        this.state = {
            ONNXVersion: {value: '1.2', label: '1.2 (opset V7)'},
            console: '', 
            currentStep: Step.Idle,
            error, 
            framework: '',
            outputNames: '',
        };
        log.info("Convert view is created.");
    }

    public UNSAFE_componentWillReceiveProps(nextProps: IComponentProperties){
        if(nextProps.file && nextProps.file.path) {
            if(!nextProps.file.path.endsWith(".onnx")) {
                this.setState({source: nextProps.file.path})
            }
            else {
                this.setState({source: ''})
            }
        }
    }
    public render() {
        const collabsibleRef: React.RefObject<Collapsible> = React.createRef();
        return (
            <div className='ConvertView'>
                <div className='ConvertViewControls'>
                    {this.getView()}
                </div>
                { this.state.console &&
                    <Collapsible ref={collabsibleRef} label='Console output'>
                        <pre className='ConverterViewConsole'>
                            {this.state.console}
                        </pre>
                    </Collapsible>
                }
            </div>
        )
    }

    private initializeState() {
        this.setState({ currentStep: Step.Idle, console: '', error: undefined, framework: ''});
    }

    private getView() {
        const { error } = this.state;
        if (error) {
            const message = typeof error === 'string' ? error : (`${error.stack ? `${error.stack}: ` : ''}${error.message}`);
            // tslint:disable-next-line:no-console
            console.log(message);
            return <MessageBar messageBarType={MessageBarType.error}>{message}</MessageBar>
        }
        switch (this.state.currentStep) {
            case Step.Downloading:
                return <Spinner label="Downloading Python..." />;
            case Step.GetPip:
                return <Spinner label="Getting pip in embedded Python..." />;
            case Step.CreatingVenv:
                return <Spinner label="Creating virtual environment..." />;
            case Step.InstallingRequirements:
                return <Spinner label="Downloading and installing requirements...(This may take more than 5 minutes)" />;
            case Step.Converting:
                return <Spinner label="Converting..." />;
        }
        this.localPython = this.localPython || getLocalPython();
        if (!this.localPython || !fs.existsSync(path.join(winmlDataFolder, '/python/PIResult.txt'))) {
            return this.pythonChooser();
        }
        return this.converterView();
    }

    private printMessage = (message: string) => {
        log.info(message);
        this.setState((prevState) => ({
            ...prevState,
            console: prevState.console.concat(message),
        }))
    }

    private logError = (error: string | Error) => {
        const message = typeof error === 'string' ? error : (`${error.stack ? `${error.stack}: ` : ''}${error.message}`);
        log.error(message)
    }

    private printError = (error: string | Error) => {
        const message = typeof error === 'string' ? error : (`${error.stack ? `${error.stack}: ` : ''}${error.message}`);
        this.printMessage(message)
        log.info(message);
        this.setState({ currentStep: Step.Idle, error, });
    }

    // tslint:disable-next-line:member-ordering
    private outputListener = {
        stderr: this.printMessage,
        stdout: this.printMessage,
    };

    private pythonChooser = () => {
        const options = [
            { key: '__download', text: 'Download a new Python binary to be used exclusively by the WinML Dashboard' } as IChoiceGroupOption
            // { key: '__Skip', text: 'Skip'} as IChoiceGroupOption
        ];
        const pythonDialogOptions = {
            message: '',
            title: 'python environment installation',
        }
        const onChange = async (ev: React.FormEvent<HTMLInputElement>, option: IChoiceGroupOption) => {
            // Clear console output
            this.setState({console: '',})
            try {
                log.info("downloading python environment is selected.");
                this.setState({ currentStep: Step.Downloading });
                this.printMessage('start downloading python\n')
                await downloadPython();
                this.setState({ currentStep: Step.GetPip });
                this.printMessage('start downloading pip\n')
                await downloadPip(this.outputListener);
                log.info("start downloading python environment.");
                this.setState({ currentStep: Step.InstallingRequirements });
                await pip(['install', packagedFile('libsvm-3.22-cp36-cp36m-win_amd64.whl')], this.outputListener);
                await pip(['install', '-r', packagedFile('requirements.txt'), '--no-warn-script-location'], this.outputListener);
                log.info("python environment is installed successfully");
                fs.writeFile(path.join(winmlDataFolder, '/python/PIResult.txt'), 'Python is installed successfully!', (err) => {
                    if (err) { throw err; }
                    log.info('PIResult.txt Saved!');
                  });
                this.setState({ currentStep: Step.Idle });
                pythonDialogOptions.message = 'python environment is installed successfully!'
                require('electron').remote.dialog.showMessageBox(require('electron').remote.getCurrentWindow(), pythonDialogOptions)
            } catch (error) {
                // if python install failed, give opportunity to reinstall 
                log.info("installation of python environment failed");
                this.printError(error);
                pythonDialogOptions.message = 'installation of python environment failed!'
                require('electron').remote.dialog.showMessageBox(require('electron').remote.getCurrentWindow(), pythonDialogOptions)
                return;
            }
            
        }
        // TODO Options to reinstall environment or update dependencies
        return (
            <ChoiceGroup
                options={options}
                onChange={onChange}
            />
        );
    }

    private converterView = () => {
        const FrameworkOptions = [
            { value: 'Coreml', label: 'Coreml' },
            { value: 'Keras', label: 'Keras' },
            { value: 'scikit-learn', label: 'scikit-learn' },
            { value: "xgboost", label: 'xgboost' },
            { value: 'libSVM', label: 'libSVM' },
            { value: 'TensorFlow', label: 'TensorFlow' },
          ];
        const ONNXVersionOptions = [
            { value: '1.2', label: '1.2 (opset V7)' },
            { value: '1.3', label: '1.3 (opset V8)' },
            { value: '1.5', label: '1.5 (opset V10)' },
            { value: '1.6', label: '1.6 (opset V11)' },
            { value: '1.7', label: '1.7 (opset V12)' },
            { value: '1.8', label: '1.8 (opset V13)' },
        ]
        return (
            <div className="ModelConvert">
                <div className='DisplayFlex'>
                    <label className='label-left-align'>Model to convert: </label>
                    <TextField id='modelToConvert' placeholder='Path' value={this.state.source}  onChanged={this.setSource} />
                    <DefaultButton id='ConverterModelInputBrowse' text='Browse' onClick={this.browseSource}/>
                </div>
                <br />
                <div className='DisplayFlex'>
                    <label className='label-left-align'>Source Framework: </label>
                    <Select className='FrameworkOptions'
                        value={this.newOption(this.state.framework)}
                        onChange={this.setFramework}
                        options={FrameworkOptions}
                    />
                    <label className='label-center-align'>ONNX Version: </label>
                    <Select className='ONNXVersionOptions'
                        value={this.newOption(this.state.ONNXVersion.label)}
                        onChange={this.setONNXVersion}
                        options={ONNXVersionOptions}
                    />
                </div>
                <br />
                <div className={this.state.framework === 'TensorFlow' ? ' ' : 'hidden'}>
                    <div className='DisplayFlex'>
                        <label className='label-left-align'>Output Names: </label>
                        <TextField id='outputNames' className='outputNames' placeholder='output:0 output:1' value={this.state.outputNames}  onChanged={this.setOutputNames} />
                    </div>
                </div>
                <DefaultButton id='ConvertButton' text='Convert' disabled={!this.state.source || !this.state.framework} onClick={this.convert}/>
            </div>
        );
    }
    
    private newOption = (framework: string):ISelectOpition => {
        return {
            label: framework,
            value: framework,
        }
    }
    private setOutputNames = (outputNames: string) => {
        this.setState({outputNames})
    }

    private setONNXVersion = (ONNXVersion: ISelectOpition) => {
        this.setState({ONNXVersion})
    }

    private setFramework = (framework: ISelectOpition) => {
        this.setState({framework: framework.value})
    }

    private setSource = (source?: string) => {
        this.setState({ source })
    }

    private browseSource = () => {
        const openDialogOptions = {
            properties: Array<'openFile'>('openFile'),
        };
        showNativeOpenDialog(openDialogOptions)
            .then((filePaths) => {
                if (filePaths) {
                    this.setSource(filePaths[0]);
                }
            });
    }

    private deleteCacheFile = (filePath: string) => {
        fs.stat(filePath, (err, stats) => {
            if (err) {
                log.info(err);
                return;
            }
            fs.unlink(filePath, (error: NodeJS.ErrnoException) => {
                 if(error) {
                    log.info(error);
                    return;
                 }
                 log.info(filePath + ' deleted successfully');
            });  
         });
    }

    private convert = async () => {
        this.initializeState();
        
        const convertDialogOptions = {
            message: '',
            title: 'convert result',
        }
        log.info("start to convert " + this.state.source);

        this.setState({ currentStep: Step.Converting });
        try {
            await python([packagedFile('convert.py'), this.state.source!, 
                                                    this.state.framework, 
                                                    this.state.ONNXVersion.value,
                                                    this.state.outputNames, 
                                                    packagedFile('tempConvertResult.onnx')], {}, this.outputListener);
        } catch (e) {
            this.logError(e);
            this.printMessage("\n------------------------------------\nConversion failed!\n")
            this.setState({ currentStep: Step.Idle});
            log.info("Conversion of " + this.state.source + " failed.");
            convertDialogOptions.message = 'convert failed!'
            require('electron').remote.dialog.showMessageBox(require('electron').remote.getCurrentWindow(), convertDialogOptions)
            return;
        }
        // Convert successfully
        log.info(this.state.source + " is converted successfully.");
        convertDialogOptions.message = 'Converted successfully!'
        require('electron').remote.dialog.showMessageBox(require('electron').remote.getCurrentWindow(), convertDialogOptions)
        this.setState({ currentStep: Step.Idle, source: undefined, console:"Converted successfully!!"});

        const destination = await showNativeSaveDialog({ filters: [{ name: 'ONNX model', extensions: ['onnx'] }, { name: 'ONNX text protobuf', extensions: ['prototxt'] }] });
        if (!destination) {
            this.deleteCacheFile(packagedFile('tempConvertResult.onnx'));
            return;
        }
        else {
            fs.copyFileSync(packagedFile('tempConvertResult.onnx'), destination);
        }
        this.deleteCacheFile(packagedFile('tempConvertResult.onnx'));
        
        this.setState({ currentStep: Step.Idle, source: undefined, console:"Converted successfully!! \n Saved to " + destination + "\n ONNX file loaded."});
        // TODO Show dialog (https://developer.microsoft.com/en-us/fabric#/components/dialog) asking whether we should open the converted model
        this.props.setFile(fileFromPath(destination));
        this.props.setSaveFileName(destination);
    }
}

const mapStateToProps = (state: IState) => ({
    file: state.file,
});

const mapDispatchToProps = {
    setFile,
    setSaveFileName,
}

export default connect(mapStateToProps, mapDispatchToProps)(ConvertView);
