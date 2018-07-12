import * as React from 'react';

interface IComponentProperties {
    file?: File,
}

interface IComponentState {
    fileUrl: string,
    netronUrl: string,
}

export default class Netron extends React.Component<IComponentProperties, IComponentState> {
    constructor(props: IComponentProperties) {
        super(props);
        this.state = this.createURLs(props.file);
    }

    public render() {
        return (
            <object className='Netron' type='text/html' data={this.state.netronUrl}>
                Netron visualizer
            </object>
        );
    }

    public componentWillReceiveProps(nextProps: IComponentProperties) {
        this.setState(this.createURLs(nextProps.file));
    }

    private createURLs(file?: File) {
        if (this.state && this.state.fileUrl) {
            URL.revokeObjectURL(this.state.fileUrl);
        }
        let fileUrl = '';
        let netronUrl = 'static/Netron/';
        if (file) {
            fileUrl = URL.createObjectURL(file);
            netronUrl = `${netronUrl}?url=${fileUrl}&identifier=${file.name}`;  // FIXME escape and format properly
        }
        return { fileUrl, netronUrl };
    }
}
