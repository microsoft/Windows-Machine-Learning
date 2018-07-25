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
    metadataProps: IMetadataProps,
    updateMetadataProps: typeof updateMetadataProps,
}

class RightPanelComponent extends React.Component<IComponentProperties, {}> {
    public render() {
        return (
            <div>
                <Label>Model</Label>
                <div className='Panel'>
                    <Collapsible label='Model metadata properties'>
                        <KeyValueEditor actionCreator={updateMetadataProps} getState={this.getMetadataPropsFromState} schema={MetadataSchema} />
                    </Collapsible>
                </div>
            </div>
        );
    }

    private getMetadataPropsFromState = (state: IState) => state.metadataProps;
}

const mapStateToProps = (state: IState) => ({
    metadataProps: state.metadataProps,
});

const mapDispatchToProps = {
    updateMetadataProps,
}

const RightPanel = connect(mapStateToProps, mapDispatchToProps)(RightPanelComponent);
export default RightPanel;
