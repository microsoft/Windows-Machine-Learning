import * as React from 'react';
import { connect } from 'react-redux';

import { DefaultButton } from 'office-ui-fabric-react/lib/Button';

import IState from '../../datastore/state';

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

enum Step {
    Idle,
    Running,
    Success,
}

interface IComponentState {
    console: string,
    currentStep: Step,
    input: string,
    model: string,


}
class RunView extends React.Component<IComponentProperties, IComponentState> {
    constructor(props: IComponentProperties) {
        super(props);
        this.state = {
            console: '',
            currentStep: Step.Idle,
            input: '',
            model: ''
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
                <DefaultButton id='RunButton' text='Run' disabled={!this.state.model} onClick={this.execModelRunner}/>
            </div>
        )
    }

    private setModel = (model: string) => {
        this.setState({ model })
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
        const paramters = ['-model', this.state.model];
        try {
            await execFilePromise(modelRunnerPath, paramters, {}, this.outputListener);
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