import * as React from 'react';
import { connect } from 'react-redux';

import { DefaultButton } from 'office-ui-fabric-react/lib/Button';
import { ChoiceGroup, IChoiceGroupOption } from 'office-ui-fabric-react/lib/ChoiceGroup';
// import { MessageBar, MessageBarType } from 'office-ui-fabric-react/lib/MessageBar';
import { Spinner } from 'office-ui-fabric-react/lib/Spinner';
import { TextField } from 'office-ui-fabric-react/lib/TextField';
import Select from 'react-select';

import Collapsible from '../../components/Collapsible';
import { setFile, setSaveFileName } from '../../datastore/actionCreators';
import IState from '../../datastore/state';
import { packagedFile } from '../../native/appData';
import { fileFromPath, showNativeOpenDialog, showNativeSaveDialog } from '../../native/dialog';
import { downloadPip, downloadPython, getLocalPython, pip, python } from '../../native/python';
import { isWeb } from '../../native/util';


import './View.css';

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
    console: string,
    currentStep: Step,
    error?: Error | string,
    framework: string,
    pythonReinstall: boolean,
    source?: string,
}

class ConvertView extends React.Component<IComponentProperties, IComponentState> {
    private localPython?: string;

    constructor(props: IComponentProperties) {
        super(props);
        const error = isWeb() ? "The converter can't be run in the web interface" : undefined;
        this.state = { 
            console: '', 
            currentStep: Step.Idle,
            error, 
            framework: '',
            pythonReinstall: false
        };
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
        this.setState({ currentStep: Step.Idle, console: '', error: undefined, framework: '', pythonReinstall: false});
    }

    private getView() {
        // const { error } = this.state;
        // if (error) {
        //     const message = typeof error === 'string' ? error : (`${error.stack ? `${error.stack}: ` : ''}${error.message}`);
        //     this.printMessage(message)
        //     this.setState({pythonReinstall: true});
        //     // return <MessageBar messageBarType={MessageBarType.error}>{message}</MessageBar>
        // }
        switch (this.state.currentStep) {
            case Step.Downloading:
                return <Spinner label="Downloading Python..." />;
            case Step.GetPip:
                return <Spinner label="Getting pip in embedded Python..." />;
            case Step.CreatingVenv:
                return <Spinner label="Creating virtual environment..." />;
            case Step.InstallingRequirements:
                return <Spinner label="Downloading and installing requirements..." />;
            case Step.Converting:
                return <Spinner label="Converting..." />;
        }
        this.localPython = this.localPython || getLocalPython();
        if (!this.localPython || this.state.pythonReinstall) {
            return this.pythonChooser();
        }
        return this.converterView();
    }

    private printMessage = (message: string) => {
        this.setState((prevState) => ({
            ...prevState,
            console: prevState.console.concat(message),
        }))
    }

    private printError = (error: string | Error) => {
        const message = typeof error === 'string' ? error : (`${error.stack ? `${error.stack}: ` : ''}${error.message}`);
        this.printMessage(message)
        this.setState({ currentStep: Step.Idle, error, pythonReinstall:true });
    }

    // tslint:disable-next-line:member-ordering
    private outputListener = {
        stderr: this.printMessage,
        stdout: this.printMessage,
    };

    private pythonChooser = () => {
        const options = [
            { key: '__download', text: 'Download a new Python binary to be used exclusively by the WinML Dashboard' } as IChoiceGroupOption,
            { key: '__Skip', text: 'Skip'} as IChoiceGroupOption
        ];
        const onChange = async (ev: React.FormEvent<HTMLInputElement>, option: IChoiceGroupOption) => {
            // Clear console output
            this.setState({console: ''})
            try {
                if (option.key === '__download') {
                    this.setState({ currentStep: Step.Downloading });
                    await downloadPython();
                    this.setState({ currentStep: Step.GetPip });
                    await downloadPip(this.outputListener);
                } else {
                    this.setState({pythonReinstall: false})
                    return;
                }
                this.setState({ currentStep: Step.InstallingRequirements });
                await pip(['install', '-r', packagedFile('requirements.txt')], this.outputListener);
                this.setState({ currentStep: Step.Idle });
            } catch (error) {
                this.printError(error);
            }
            // reset pythonReinstall
            this.setState({pythonReinstall: false})
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
        const options = [
            { value: 'Coreml', label: 'Coreml' },
            { value: 'Keras', label: 'Keras' },
            { value: 'scikit-learn', label: 'scikit-learn' },
            { value: "xgboost", label: 'xgboost' },
            { value: 'libSVM', label: 'libSVM' }
          ];
        return (
            <div>
                <div className='DisplayFlex ModelConvertBrowser'>
                    <TextField id='modelToConvert' placeholder='Path' value={this.state.source || this.props.file && this.props.file.path} label='Model to convert' onChanged={this.setSource} />
                    <DefaultButton id='ConverterModelInputBrowse' text='Browse' onClick={this.browseSource}/>
                </div>
                <div className='Frameworks'>
                    <p>Source Framework: </p>
                    <Select
                        value={this.newOption(this.state.framework)}
                        onChange={this.setFramework}
                        options={options}
                    />
                </div>
                <DefaultButton id='ConvertButton' text='Convert' disabled={!this.state.source || !this.state.framework} onClick={this.convert}/>
            </div>
        );
    }

    private newOption = (framework: string):ISelectOpition => {
        return {
            label: framework,
            value: framework
        }
    }

    private setFramework = (framework: ISelectOpition) => {
        this.setState({framework: framework.value})
        // tslint:disable-next-line:no-console
        console.log(framework.value)
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

    private convert = async () => {
        this.initializeState();
        const source = this.state.source!;
        const framework = this.state.framework;
        const destination = await showNativeSaveDialog({ filters: [{ name: 'ONNX model', extensions: ['onnx'] }, { name: 'ONNX text protobuf', extensions: ['prototxt'] }] });
        if (!destination) {
            return;
        }
        if (!framework) {
            return;
        }
        this.setState({ currentStep: Step.Converting });
        try {
            await python([packagedFile('convert.py'), source, framework, destination], {}, this.outputListener);
        } catch (e) {
            this.printError(e);
            return;
        }

        // Convert successfully
        this.setState({ currentStep: Step.Idle, source: undefined, console:"convert successfully!!"});
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
