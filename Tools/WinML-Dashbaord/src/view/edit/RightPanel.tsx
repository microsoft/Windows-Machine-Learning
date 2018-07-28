import { Label } from 'office-ui-fabric-react/lib/Label';
import * as React from 'react';
import { connect } from 'react-redux';

import Collapsible from '../../components/Collapsible';
import KeyValueEditor from '../../components/KeyValueEditor'
import { updateMetadataProps } from '../../datastore/actionCreators';
import IState, { IMetadataProps } from '../../datastore/state';
import MetadataSchema from '../../schema/Metadata';

import './Panel.css';

interface IComponentProperties {
    // Redux properties
    nodes: any,
    metadataProps: IMetadataProps,
    updateMetadataProps: typeof updateMetadataProps,
}

class RightPanel extends React.Component<IComponentProperties, {}> {
    public render() {
        if (this.props.nodes === null) {
            return (
                <Label className='FormatIsNotOnnx'>To support editing, convert the model to ONNX first.</Label>
            );
        }

        return (
            <div>
                <Label>Model</Label>
                <div className='Panel'>
                    <Collapsible label='Properties'>
                        <KeyValueEditor getState={this.getPropertiesFromState} schema={{ type: 'object' }} />
                    </Collapsible>
                    <Collapsible label='Metadata properties'>
                        <KeyValueEditor actionCreator={updateMetadataProps} getState={this.getMetadataPropsFromState} schema={MetadataSchema} />
                    </Collapsible>
                </div>
            </div>
        );
    }

    private getMetadataPropsFromState = (state: IState) => state.metadataProps;
    private getPropertiesFromState = (state: IState) => state.properties;
}

const mapStateToProps = (state: IState) => ({
    metadataProps: state.metadataProps,
    nodes: state.nodes,
    properties: state.properties,
});

const mapDispatchToProps = {
    updateMetadataProps,
}

export default connect(mapStateToProps, mapDispatchToProps)(RightPanel);
