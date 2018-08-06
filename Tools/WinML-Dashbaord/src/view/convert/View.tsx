import { DefaultButton } from 'office-ui-fabric-react/lib/Button';
import { ChoiceGroup, IChoiceGroupOption } from 'office-ui-fabric-react/lib/ChoiceGroup';
import { MessageBar, MessageBarType } from 'office-ui-fabric-react/lib/MessageBar';
import { Spinner } from 'office-ui-fabric-react/lib/Spinner';
import * as path from 'path';
import * as React from 'react';

import { winmlDataFolder } from '../../persistence/appData';
import { downloadPip, downloadProtobuf, downloadPython, getLocalPython, getPythonBinaries, installVenv, pip } from '../../python/python';

import './View.css';

enum InstallationStep {
    NotInstalling,
    Downloading,
    GetPip,
    GetProtobuf,
    CreatingVenv,
    InstallingRequirements,
}

interface IComponentState {
    installationStep: InstallationStep,
    error?: Error,
}

export default class ConvertView extends React.Component<{}, IComponentState> {
    private venvPython: string | undefined;

    constructor(props: {}) {
        super(props);
        this.state = {
            error: winmlDataFolder === '/' ? Error("The converter can't be run in the web interface") : undefined,
            installationStep: InstallationStep.NotInstalling,
        };
    }

    public render() {
        return (
            <div className='ConvertView'>
                {this.getView()}
            </div>
        )
    }

    private getView() {
        const { error } = this.state;
        if (error) {
            return <MessageBar messageBarType={MessageBarType.error}>{`${error.stack ? `${error.stack}: ` : ''}${error.message}`}</MessageBar>
        }
        switch (this.state.installationStep) {
            case InstallationStep.Downloading:
                return <Spinner label="Downloading Python..." />;
            case InstallationStep.GetPip:
                return <Spinner label="Getting pip to embedded Python..." />;
            case InstallationStep.GetProtobuf:
                return <Spinner label="Getting protobuf..." />;
            case InstallationStep.CreatingVenv:
                return <Spinner label="Creating virtual environment..." />;
            case InstallationStep.InstallingRequirements:
                return <Spinner label="Downloading and installing requirements..." />;
        }
        this.venvPython = this.venvPython || getLocalPython();
        if (!this.venvPython) {
            return this.pythonChooser();
        }
        return <DefaultButton style={{ height: '100vh', width: '100%' }} text='Open file' onClick={this.convert}/>;
    }

    private pythonChooser = () => {
        const binaries = getPythonBinaries();
        const options = binaries.map((key) => key ? { key, text: key } : { key: '__download', text: 'Download a new Python binary to be used exclusively by the WinML Dashboard' });
        const onChange = async (ev: React.FormEvent<HTMLInputElement>, option: IChoiceGroupOption) => {
            try {
                if (option.key === '__download') {
                    this.setState({ installationStep: InstallationStep.Downloading });
                    await downloadPython();
                    this.setState({ installationStep: InstallationStep.GetPip });
                    await downloadPip();
                    this.setState({ installationStep: InstallationStep.GetProtobuf });
                    await downloadProtobuf();
                } else {
                    this.setState({ installationStep: InstallationStep.CreatingVenv });
                    await installVenv(option.key);
                }
                this.setState({ installationStep: InstallationStep.InstallingRequirements });
                await pip('install', '-r', path.join(__filename, 'requirements.txt'));
                this.setState({ installationStep: InstallationStep.NotInstalling });
            } catch (error) {
                this.setState({ error, installationStep: InstallationStep.NotInstalling });
            }
        }
        return (
            <ChoiceGroup
                options={options}
                label={binaries[0] ? 'Suitable Python versions were found in the system. Pick one to be used by the converter.' : 'No suitable Python versions were found in the system.'}
                onChange={onChange}
            />
        );
    }

    private convert = () => {
        // Dialog box
        // python
        // tslint:disable-next-line:no-console
        console.log('Convert')
    }
}
