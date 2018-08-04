// tslint:disable:no-console
import { DefaultButton } from 'office-ui-fabric-react/lib/Button';
import { ChoiceGroup, IChoiceGroupOption } from 'office-ui-fabric-react/lib/ChoiceGroup';
import { MessageBar, MessageBarType } from 'office-ui-fabric-react/lib/MessageBar';
import { Spinner } from 'office-ui-fabric-react/lib/Spinner';
import * as React from 'react';

import { winmlDataFoler } from '../../persistence/appData';
import { downloadPython, getPythonBinaries, getVenvPython, installVenv, localPython } from '../../python/python';

import './View.css';

enum InstallationStep {
    NotInstalling,
    Downloading,
    Installing,
}

interface IComponentState {
    installationStep: InstallationStep,
    error?: Error,
}

export default class ConvertView extends React.Component<{}, IComponentState> {
    private venvPython: string;

    constructor(props: {}) {
        super(props);
        const state: IComponentState = { installationStep: InstallationStep.NotInstalling };
        if (winmlDataFoler === '/') {
            state.error = Error("The converter can't be run in the web interface");
        }
        this.state = state;
    }

    public render() {
        return (
            <div className='ConvertView'>
                {this.getView()}
            </div>
        )
    }

    private getView() {
        if (this.state.error) {
            return <MessageBar messageBarType={MessageBarType.error}>{this.state.error.message}</MessageBar>
        }
        switch (this.state.installationStep) {
            case InstallationStep.Downloading:
                return <Spinner label="Downloading Python..." />;
            case InstallationStep.Installing:
                return <Spinner label="Downloading and installing converter..." />;
        }
        this.venvPython = this.venvPython || getVenvPython();
        if (!this.venvPython) {
            return this.pythonChooser();
        }
        return <DefaultButton style={{ height: '100vh', width: '100%' }} text='Open file' onClick={this.convert}/>;
    }

    private pythonChooser = () => {
        const binaries = getPythonBinaries();
        console.log(binaries);
        const options = binaries.map((key) => key ? { key, text: key } : { key: '__download', text: 'Download a new Python binary to be used exclusively by the WinML Dashboard' });
        const onChange = (ev: React.FormEvent<HTMLInputElement>, option: IChoiceGroupOption) => {
            if (option.key === '__download') {
                this.setState({ installationStep: InstallationStep.Downloading });
                downloadPython(() => this.installVenv(localPython), (err) => this.setState({ error: err }));
            } else {
                this.installVenv(option.key);
            }
        }
        return (
            <ChoiceGroup
                options={options}
                label={binaries[0] ? 'No suitable Python versions were found in the system.' : 'Suitable Python versions were found in the system. Pick one to be used by the converter.'}
                onChange={onChange}
            />
        );
    }

    private installVenv = async (targetPython: string) => {
        this.setState({ installationStep: InstallationStep.Installing });
        try {
            await installVenv(targetPython);
            this.setState({ installationStep: InstallationStep.NotInstalling });
        } catch (error) {
            this.setState({ error, installationStep: InstallationStep.NotInstalling });
        }
    }

    private convert = () => {
        // Dialog box
        // python
        console.log('Convert')
    }
}
