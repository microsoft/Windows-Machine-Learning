import * as React from 'react';
import { connect } from 'react-redux';


import { setFile } from '../../datastore/actionCreators';
import { ModelProtoSingleton } from "../../datastore/proto/modelProto";
import IState from '../../datastore/state';

import Select from 'react-select';

import { DefaultButton } from 'office-ui-fabric-react/lib/Button';
import { MessageBar, MessageBarType } from 'office-ui-fabric-react/lib/MessageBar'
import { Spinner } from 'office-ui-fabric-react/lib/Spinner';
import { TextField } from 'office-ui-fabric-react/lib/TextField';

import { clearLocalDebugDir, getLocalDebugDir } from '../../native/appData';
import { packagedFile } from '../../native/appData';
import { fileFromPath } from '../../native/dialog';
import { save } from "../../native/dialog";
import { showNativeOpenDialog } from '../../native/dialog';
import { execFilePromise } from '../../native/python';

import * as path from 'path';

import './View.css';

import Collapsible from '../../components/Collapsible';

import log from 'electron-log';

const modelRunnerPath = packagedFile('WinMLRunner.exe');

interface IComponentProperties {
    file: File,
    setFile: typeof setFile,
}

interface ISelectOption<T> {
    label: T;
    value: T;
}

enum Step {
    Idle,
    Running,
    Success,
}

enum Capture {
    None = "None",
    Perf = "Perf",
    Debug = "Debug",
}

enum Device {
    CPU = 'CPU',
    GPU = 'GPU',
    GPUHighPerformance = 'GPUHighPerformance',
    GPUMinPower = 'GPUMinPower',
}

interface IComponentState {
    capture: Capture,
    console: string,
    currentStep: Step,
    device: Device,
    inputPath: string,
    inputType: string,
    model: string,
    parameters: string[],
}

class RunView extends React.Component<IComponentProperties, IComponentState> {
    constructor(props: IComponentProperties) {
        super(props);
        this.state = {
            capture: Capture.None,
            console: '',
            currentStep: Step.Idle,
            device: Device.CPU,
            inputPath: '',
            inputType: '',
            model: '',
            parameters: [],
        }
        log.info("Run view is created.");
    }
    public UNSAFE_componentWillReceiveProps(nextProps: IComponentProperties) {
        if(nextProps.file.path && nextProps.file.path) {
            if(!nextProps.file.path.endsWith(".onnx")){
                this.setState({model: ''}, () => {this.setParameters()})
                return;
            }
            if(!(this.props.file && this.props.file.path) || this.props.file.path !== nextProps.file.path){
                this.setState({model: nextProps.file.path}, () => {this.setParameters()})
            }
        }
    }
    public render() {
        const collabsibleRef: React.RefObject<Collapsible> = React.createRef();
        return (
            <div className='RunView'>
                <div className='RunViewControls'>
                    {this.getView()}
                </div>
                { this.state.console &&
                    <Collapsible ref={collabsibleRef} label='Console output'>
                        <pre className='RunViewConsole'>
                            {this.state.console}
                        </pre>
                    </Collapsible>
                }
            </div>
        )
    }

    private newOption<T> (item: T):ISelectOption<T> {
        return {
            label: item,
            value: item
        }
    }

    private newOptions<T> (items: T[]):Array<ISelectOption<T>> {
        const options = [];
        for (const item of items) {
            options.push({ label: item, value: item });
        }
        return options;
    }

    private getView = () => {
        const osInfo = require('os').release()
        log.info(osInfo);
        if(osInfo < '10.0.17763') {
            const message = 'This functionality is available on Windows 10 October 2018 Update (1809) or newer version of OS.'
            return <MessageBar messageBarType={MessageBarType.error}>{message}</MessageBar>
        }
        switch(this.state.currentStep) {
            case Step.Running:
                return <Spinner label="Running..." />;
        }
        return (
            <div>
                <div className='ArgumentsControl'>
                    {this.getArgumentsView()}
                </div>
                <TextField id='paramters' readOnly={true} placeholder='paramters' value={this.state.parameters.join(' ')} label='Parameters to WinMLRunner' onChanged={this.updateParameters} />
                <DefaultButton id='RunButton' text='Run' disabled={!this.state.model} onClick={this.execModelRunner}/>
            </div>
        )
    }

    private getArgumentsView = () => {
        return (
            <div className="Arguments">
                <div className='DisplayFlex ModelPath'>
                    <label className="label">Model Path: </label>
                    <TextField id='modelToRun' placeholder='Model Path' value={this.state.model} onChanged={this.setModel} />
                    <DefaultButton id='ModelPathBrowse' text='Browse' onClick={this.browseModel}/>
                </div>
                <br />
                <div className='DisplayFlex Device'>
                    <label className="label">Devices: </label>
                    <Select className="DropdownOption"
                        value={this.newOption(this.state.device)}
                        onChange={this.setDevice}
                        options={this.newOptions(Object.keys(Device))}
                    />
                </div>
                <br />
                <div className='DisplayFlex Capture'>
                    <label className="label">Capture: </label>
                    <Select className="DropdownOption"
                        value={this.newOption(this.state.capture)}
                        onChange={this.setCapture}
                        options={this.newOptions(Object.keys(Capture))}
                    />
                </div>
                <br />
                <div className='DisplayFlex Input'>
                    <label className="label">Input Path: </label>
                    <TextField id='InputPath' placeholder='(Optional) image/csv Path' value={this.state.inputPath} onChanged={this.setInputPath} />
                    <DefaultButton id='InputPathBrowse' text='Browse' onClick={this.browseInput}/>
                </div>
            </div>
        )
    }

    private updateParameters = (parameters: string) => {
        parameters = parameters.replace(/\s+/g,' ').trim();
        this.setState({parameters: parameters.split(' ')})
    }

    private setModel = (model: string) => {
        this.setState({ model }, () => {this.setParameters()} );
        this.props.setFile(fileFromPath(this.state.model));
    }

    private setDevice = (device: ISelectOption<Device>) => {
        this.setState({device: device.value}, () => {this.setParameters()})
    }

    private setCapture = (capture: ISelectOption<Capture>) => {
        this.setState({capture: capture.value}, () => {this.setParameters()})
    }

    private setInputPath = (inputPath: string) => {
        this.setState({inputPath}, () => {this.setParameters()})
    }

    private getDebugModelPath() {
        return path.join(getLocalDebugDir(), path.basename(this.state.model));
    }

    private setParameters = () => {
        const tempParameters = []
        if(this.state.model) {
            tempParameters.push('-model')
            if (this.state.capture === Capture.Debug) {
                tempParameters.push(this.getDebugModelPath())
            } else {
                tempParameters.push(this.state.model)
            }
        }
        if(this.state.device) {
            switch(this.state.device) {
                case Device.CPU:
                    tempParameters.push('-CPU');
                    break;
                case Device.GPU:
                    tempParameters.push('-GPU');
                    break;
                case Device.GPUHighPerformance:
                    tempParameters.push('-GPUHighPerformance');
                    break;
                case Device.GPUMinPower:
                    tempParameters.push('-GPUMinPower');
                    break;
            }
        }
        if(this.state.inputPath) {
            tempParameters.push('-input')
            tempParameters.push(this.state.inputPath)
        }

        if(this.state.capture === Capture.Perf) {
            tempParameters.push('-perf')
        }

        this.setState({parameters: tempParameters})
    }
    private browseModel = () => {
        const openDialogOptions = {
            properties: Array<'openFile'>('openFile'),
        };
        showNativeOpenDialog(openDialogOptions)
            .then((filePaths) => {
                if (filePaths) {
                    this.setModel(filePaths[0]);
                }
            });
    }

    private browseInput = () => {
        const openDialogOptions = {
            properties: Array<'openFile'>('openFile'),
        };
        showNativeOpenDialog(openDialogOptions)
            .then((filePaths) => {
                if (filePaths) {
                    this.setInputPath(filePaths[0]);
                }
            });
    }

    private logError = (error: string | Error) => {
        const message = typeof error === 'string' ? error : (`${error.stack ? `${error.stack}: ` : ''}${error.message}`);
        log.error(message)
    }

    private printMessage = (message: string) => {
        this.setState((prevState) => ({
            ...prevState,
            console: prevState.console.concat(message),
        }))
    }

    // tslint:disable-next-line:member-ordering
    private outputListener = {
        stderr: this.printMessage,
        stdout: this.printMessage,
    };

    private execModelRunner = async() => {
        log.info("start to run " + this.state.model);

        // serialize debug onnx model
        if (this.state.capture === Capture.Debug) {
            clearLocalDebugDir();
            save(ModelProtoSingleton.serialize(true), this.getDebugModelPath());
        }

        this.setState({
            console: '',
            currentStep: Step.Running,
        });
        const runDialogOptions = {
            message: '',
            title: 'run result',
        }
        try {
            await execFilePromise(modelRunnerPath, this.state.parameters, {}, this.outputListener);
        } catch (e) {
            this.logError(e);
            this.printMessage("\n---------------------------\nRun Failed!\n")
            
            log.info(this.state.model + " is failed to run");
            this.setState({
                currentStep: Step.Idle,
            });
            runDialogOptions.message = 'Run failed! See console log for details.'
            require('electron').remote.dialog.showMessageBox(runDialogOptions)
            return;
        }
        this.setState({
            currentStep: Step.Success,
        });
        runDialogOptions.message = 'Run successful';
        require('electron').remote.dialog.showMessageBox(runDialogOptions)
        log.info(this.state.model + " run successful");
    }
}

const mapStateToProps = (state: IState) => ({
    file: state.file,
});

const mapDispatchToProps = {
    setFile,
}

export default connect(mapStateToProps, mapDispatchToProps)(RunView);