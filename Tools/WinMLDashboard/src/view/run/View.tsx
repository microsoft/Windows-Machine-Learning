import * as React from 'react';
import { connect } from 'react-redux';

import { DefaultButton } from 'office-ui-fabric-react/lib/Button';

import IState from '../../datastore/state';


import { TextField } from 'office-ui-fabric-react/lib/TextField';
import { packagedFile } from '../../native/appData';
import { showNativeOpenDialog } from '../../native/dialog';
import { execFilePromise } from '../../native/python';
import './view.css';

const modelRunnerPath = packagedFile('WinMLRunner.exe');

interface IComponentProperties {
    file: File
}

interface IComponentState {
    console: string,
    input: string,
    model: string,
}
class RunView extends React.Component<IComponentProperties, IComponentState> {
    constructor(props: IComponentProperties) {
        super(props);
        this.state = {
            console: '',
            input: '',
            model: ''
        }
    }
    public render() {
        return (
            <div className='RunView'>
                <div className='ModelPath'>
                    <TextField id='modelToRun' placeholder='Model Path' value={this.state.model || this.props.file && this.props.file.path} label='Model to convert' onChanged={this.setModel} />
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
        const paramters = ['-model', this.state.model];
        try {
            await execFilePromise(modelRunnerPath, paramters, {}, this.outputListener);
        } catch (e) {
            this.printError(e);
            return;
        }
        // tslint:disable-next-line:no-console
        console.log(this.state.console);
    }
}

const mapStateToProps = (state: IState) => ({
    file: state.file,
});

export default connect(mapStateToProps)(RunView);