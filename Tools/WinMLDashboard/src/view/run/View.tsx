import * as React from 'react';
import { connect } from 'react-redux';

import { DefaultButton } from 'office-ui-fabric-react/lib/Button';

import IState from '../../datastore/state';

import Select from 'react-select';

import { Spinner } from 'office-ui-fabric-react/lib/Spinner';
import { TextField } from 'office-ui-fabric-react/lib/TextField';
import { packagedFile } from '../../native/appData';
import { showNativeOpenDialog } from '../../native/dialog';
import { execFilePromise } from '../../native/python';
import './View.css';

import Collapsible from '../../components/Collapsible';

const modelRunnerPath = packagedFile('WinMLRunner.exe');

interface IComponentProperties {
    file: File
}

interface ISelectOpition {
    label: string;
    value: string;
}

enum Step {
    Idle,
    Running,
    Success,
}

interface IComponentState {
    console: string,
    currentStep: Step,
    device: string,
    inputPath: string,
    inputType: string,
    model: string,
    parameters: string[],
}
class RunView extends React.Component<IComponentProperties, IComponentState> {
    constructor(props: IComponentProperties) {
        super(props);
        this.state = {
            console: '',
            currentStep: Step.Idle,
            device: '',
            inputPath: '',
            inputType: '',
            model: '',
            parameters: [],
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

    private newOption = (item: string):ISelectOpition => {
        return {
            label: item,
            value: item
        }
    }

    private getView() {
        switch(this.state.currentStep) {
            case Step.Running:
                return <Spinner label="Running..." />;
        }
        return (
            <div>
                <div className='DisplayFlex ModelPathBrowser'>
                    <TextField id='modelToRun' placeholder='Model Path' value={this.state.model || this.props.file && this.props.file.path} label='Model to Run' onChanged={this.setModel} />
                    <DefaultButton id='ConverterModelInputBrowse' text='Browse' onClick={this.browseSource}/>
                </div>
                <div className='ArgumentsControl'>
                    {this.getArgumentsView()}
                </div>
                <TextField id='paramters' readOnly={true} placeholder='paramters' value={this.state.parameters.join(' ')} label='paramters to model runner.exe' onChanged={this.updateParameters} />
                <DefaultButton id='RunButton' text='Run' disabled={!this.state.model} onClick={this.execModelRunner}/>
            </div>
        )
    }

    private getArgumentsView() {
        const deviceOptions = [
            { value: 'CPU', label: 'CPU' },
            { value: 'GPU', label: 'GPU' },
            { value: 'GPUHighPerformance', label: 'GPUHighPerformance' },
            { value: "GPUMinPower", label: 'GPUMinPower' }
          ];
        return (
            <div className="Arguments">
                <div className='DisplayFlex Device'>
                    <p>Devices: </p>
                    <Select
                        value={this.newOption(this.state.device)}
                        onChange={this.setDevice}
                        options={deviceOptions}
                    />  
                </div>
                <div className='DisplayFlex Input'>
                    <TextField id='InputPath' placeholder='image/csv Path' value={this.state.inputPath} onChanged={this.setInputPath} />
                    <DefaultButton id='InputPathBrowse' text='Browse' onClick={this.browseSource}/>
                </div>
            </div>
        )
    }

    private updateParameters = (parameters: string) => {
        parameters = parameters.replace(/\s+/g,' ').trim();
        this.setState({parameters: parameters.split(' ')})
    }

    private setModel = (model: string) => {
        this.setState({ model }, () => {this.setParameters()} )
    }

    private setDevice = (device: ISelectOpition) => {
        this.setState({device: device.value}, () => {this.setParameters()})
    }

    private setInputPath = (inputPath: string) => {
        this.setState({inputPath}, () => {this.setParameters()})
    }

    private setParameters = () => {
        const tempParameters = []
        tempParameters.push('-model')
        tempParameters.push(this.state.model)
        if(this.state.device) {
            switch(this.state.device) {
                case 'CPU':
                    tempParameters.push('-CPU');
                    break;
                case 'GPU':
                    tempParameters.push('-GPU');
                    break;
                case 'GPUHighPerformance':
                    tempParameters.push('-GPUHighPerformance');
                    break;
                case 'GPUMinPower':
                    tempParameters.push('-GPUMinPower');
                    break;
            }
        }
        if(this.state.inputPath) {
            tempParameters.push('-input')
            tempParameters.push(this.state.inputPath)
        }
        this.setState({parameters: tempParameters})
    }
    private browseSource = () => {
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

    private printError = (error: string | Error) => {
        const message = typeof error === 'string' ? error : (`${error.stack ? `${error.stack}: ` : ''}${error.message}`);
        this.printMessage(message)
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
        this.setState({
            console: '',
            currentStep: Step.Running,
        });
        // const paramters = ['-model', this.state.model];
        try {
            await execFilePromise(modelRunnerPath, this.state.parameters, {}, this.outputListener);
        } catch (e) {
            this.printError(e);
            this.setState({
                currentStep: Step.Idle,
            });
            return;
        }
        this.setState({
            currentStep: Step.Success,
        });
        // tslint:disable-next-line:no-console
        console.log(this.state.currentStep.toString());
    }
}

const mapStateToProps = (state: IState) => ({
    file: state.file,
});

export default connect(mapStateToProps)(RunView);