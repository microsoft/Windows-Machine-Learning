import { DefaultButton } from 'office-ui-fabric-react/lib/Button';
import { ChoiceGroup, IChoiceGroupOption } from 'office-ui-fabric-react/lib/ChoiceGroup';
import { MessageBar, MessageBarType } from 'office-ui-fabric-react/lib/MessageBar';
import { Spinner } from 'office-ui-fabric-react/lib/Spinner';
import { TextField } from 'office-ui-fabric-react/lib/TextField';
import * as React from 'react';

import Collapsible from '../../components/Collapsible';
import { showOpenDialog, showSaveDialog } from '../../native';
import { packagedFile, winmlDataFolder } from '../../persistence/appData';
import { downloadPip, downloadPython, getLocalPython, getPythonBinaries, installVenv, pip, python } from '../../python/python';

import './View.css';

enum Step {
    Idle,
    Downloading,
    GetPip,
    CreatingVenv,
    InstallingRequirements,
    Converting,
}

interface IComponentState {
    console: string,
    currentStep: Step,
    error?: Error | string,
    source?: string,
}

export default class ConvertView extends React.Component<{}, IComponentState> {
    private localPython?: string;

    constructor(props: {}) {
        super(props);
        const error = winmlDataFolder === '/' ? "The converter can't be run in the web interface" : undefined;
        this.state = { console: '', error, currentStep: Step.Idle };
    }

    public render() {
        const collabsibleRef: React.RefObject<Collapsible> = React.createRef();
        return (
            <div className='ConvertView'>
                <div className='ConvertViewSplit'>
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

    private getView() {
        const { error } = this.state;
        if (error) {
            const message = typeof error === 'string' ? error : (`${error.stack ? `${error.stack}: ` : ''}${error.message}`);
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
                return <Spinner label="Downloading and installing requirements..." />;
            case Step.Converting:
                return <Spinner label="Converting..." />;
        }
        this.localPython = this.localPython || getLocalPython();
        if (!this.localPython) {
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
        this.setState({ currentStep: Step.Idle, error });
    }

    // tslint:disable-next-line:member-ordering
    private outputListener = {
        stderr: this.printMessage,
        stdout: this.printMessage,
    };

    private pythonChooser = () => {
        const binaries = getPythonBinaries();
        const options = binaries.map((key) => key ? { key, text: key } : { key: '__download', text: 'Download a new Python binary to be used exclusively by the WinML Dashboard' });
        const onChange = async (ev: React.FormEvent<HTMLInputElement>, option: IChoiceGroupOption) => {
            try {
                if (option.key === '__download') {
                    this.setState({ currentStep: Step.Downloading });
                    await downloadPython();
                    this.setState({ currentStep: Step.GetPip });
                    await downloadPip(this.outputListener);
                } else {
                    this.setState({ currentStep: Step.CreatingVenv });
                    await installVenv(option.key, this.outputListener);
                }
                this.setState({ currentStep: Step.InstallingRequirements });
                await pip(['install', '-r', packagedFile('requirements.txt')], this.outputListener);
                this.setState({ currentStep: Step.Idle });
            } catch (error) {
                this.printError(error);
            }
        }
        // TODO Options to reinstall environment or update dependencies
        return (
            <ChoiceGroup
                options={options}
                label={binaries[0] ? 'Suitable Python versions were found in the system. Pick one to be used by the converter.' : 'No suitable Python versions were found in the system.'}
                onChange={onChange}
            />
        );
    }

    private converterView = () => {
        return (
            <div>
                <div className='DisplayFlex ModelConvertBrowser'>
                    <TextField placeholder='Path' value={this.state.source} label='Model to convert' onChanged={this.setSource} />
                    <DefaultButton id='ConverterModelInputBrowse' text='Browse' onClick={this.browseSource}/>
                </div>
                <DefaultButton id='ConvertButton' text='Convert' disabled={!this.state.source} onClick={this.convert}/>
            </div>
        );
    }

    private setSource = (source?: string) => {
        this.setState({ source })
    }

    private browseSource = () => {
        const openDialogOptions = {
            filters: [
                { name: 'CoreML model', extensions: [ 'mlmodel' ] },
                { name: 'Keras model', extensions: [ 'keras', 'h5' ] },
                { name: 'ONNX model', extensions: [ 'onnx' ] },
            ],
            properties: Array<'openFile'>('openFile'),
        };
        showOpenDialog(openDialogOptions)
            .then((filePaths) => {
                if (filePaths) {
                    this.setSource(filePaths[0]);
                }
            });
    }

    private convert = () => {
        const source = this.state.source!;
        showSaveDialog({ filters: [{ name: 'ONNX model', extensions: ['onnx'] }, { name: 'ONNX text protobuf', extensions: ['prototxt'] }] })
            .then((destination: string) => {
                if (destination) {
                    this.setState({ currentStep: Step.Converting });
                    python([packagedFile('convert.py'), source, destination], {}, this.outputListener)
                        .then(() => this.setState({ currentStep: Step.Idle, source: undefined }))
                        .catch(this.printError);
                }
            });
    }
}
