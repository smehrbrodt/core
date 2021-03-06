/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */
#ifndef INCLUDED_SW_INC_UNOSTYLE_HXX
#define INCLUDED_SW_INC_UNOSTYLE_HXX

#include <svl/style.hxx>
#include <svl/lstner.hxx>
#include <unocoll.hxx>
#include <unomap.hxx>
#include <com/sun/star/style/XStyle.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <com/sun/star/style/XStyleLoader.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XMultiPropertySet.hpp>
#include <com/sun/star/beans/XPropertyState.hpp>
#include <com/sun/star/beans/XMultiPropertyStates.hpp>
#include <cppuhelper/implbase.hxx>
#include <com/sun/star/document/XEventsSupplier.hpp>
#include <calbck.hxx>

#include <com/sun/star/style/XAutoStyleFamily.hpp>
#include <com/sun/star/style/XAutoStyles.hpp>
#include <com/sun/star/style/XAutoStyle.hpp>

#include <istyleaccess.hxx>
#include <memory>

class SwDocShell;
class SfxItemPropertySet;
class SwXStyle;
class SwStyleProperties_Impl;
class SwAutoStylesEnumImpl;
class IStyleAccess;
class SfxItemSet;
namespace sw
{
    class XStyleFamily;
};
typedef std::shared_ptr<SfxItemSet> SfxItemSet_Pointer_t;

class SwXStyleFamilies :  public cppu::WeakImplHelper
<
    css::container::XIndexAccess,
    css::container::XNameAccess,
    css::lang::XServiceInfo,
    css::style::XStyleLoader
>,
    public SwUnoCollection
{
    SwDocShell*         m_pDocShell;

    std::map<SfxStyleFamily, css::uno::Reference<css::container::XNameContainer>> m_vFamilies;
protected:
    virtual ~SwXStyleFamilies();
public:
    SwXStyleFamilies(SwDocShell& rDocShell);

    //XNameAccess
    virtual css::uno::Any SAL_CALL getByName(const OUString& Name) throw( css::container::NoSuchElementException, css::lang::WrappedTargetException, css::uno::RuntimeException, std::exception ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getElementNames() throw( css::uno::RuntimeException, std::exception ) override;
    virtual sal_Bool SAL_CALL hasByName(const OUString& Name) throw( css::uno::RuntimeException, std::exception ) override;

    //XIndexAccess
    virtual sal_Int32 SAL_CALL getCount() throw( css::uno::RuntimeException, std::exception ) override;
    virtual css::uno::Any SAL_CALL getByIndex(sal_Int32 nIndex) throw( css::lang::IndexOutOfBoundsException, css::lang::WrappedTargetException, css::uno::RuntimeException, std::exception ) override;

    //XElementAccess
    virtual css::uno::Type SAL_CALL getElementType(  ) throw(css::uno::RuntimeException, std::exception) override;
    virtual sal_Bool SAL_CALL hasElements(  ) throw(css::uno::RuntimeException, std::exception) override;

    //XStyleLoader
    virtual void SAL_CALL loadStylesFromURL(const OUString& rURL, const css::uno::Sequence< css::beans::PropertyValue >& aOptions)
        throw (css::io::IOException,
               css::uno::RuntimeException,
               std::exception) override;
    virtual css::uno::Sequence< css::beans::PropertyValue > SAL_CALL getStyleLoaderOptions() throw( css::uno::RuntimeException, std::exception ) override;

    //XServiceInfo
    virtual OUString SAL_CALL getImplementationName() throw( css::uno::RuntimeException, std::exception ) override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) throw( css::uno::RuntimeException, std::exception ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() throw( css::uno::RuntimeException, std::exception ) override;

    static css::uno::Reference<css::style::XStyle> CreateStyle(SfxStyleFamily eFamily, SwDoc& rDoc);
    // FIXME: This is very ugly as is the whole conditional paragraph style
    // hackety. Should be folded into CreateStyle hopefully one day
    static css::uno::Reference<css::style::XStyle> CreateStyleCondParagraph(SwDoc& rDoc);
};

namespace sw
{
    class ICoreFrameStyle
    {
        public:
            virtual void SetItem(enum RES_FRMATR eAtr, const SfxPoolItem& rItem) =0;
            virtual const SfxPoolItem* GetItem(enum RES_FRMATR eAtr) =0;
            virtual css::document::XEventsSupplier& GetEventsSupplier() =0;
            virtual ~ICoreFrameStyle() {};
    };
    class ICoreParagraphStyle
    {
        public:
            static const css::uno::Sequence<sal_Int8>& getUnoTunnelId();
            virtual const SwTextFormatColl* GetFormatColl() =0;
            virtual ~ICoreParagraphStyle() {};
    };
}

// access to all automatic style families
class SwXAutoStyles :
    public cppu::WeakImplHelper< css::style::XAutoStyles >,
    public SwUnoCollection
{
    SwDocShell *m_pDocShell;
    css::uno::Reference< css::style::XAutoStyleFamily > m_xAutoCharStyles;
    css::uno::Reference< css::style::XAutoStyleFamily > m_xAutoRubyStyles;
    css::uno::Reference< css::style::XAutoStyleFamily > m_xAutoParaStyles;
    virtual ~SwXAutoStyles();

public:
    SwXAutoStyles(SwDocShell& rDocShell);

    //XIndexAccess
    virtual sal_Int32 SAL_CALL getCount() throw( css::uno::RuntimeException, std::exception ) override;
    virtual css::uno::Any SAL_CALL getByIndex(sal_Int32 nIndex) throw( css::lang::IndexOutOfBoundsException, css::lang::WrappedTargetException, css::uno::RuntimeException, std::exception ) override;

    //XElementAccess
    virtual css::uno::Type SAL_CALL SAL_CALL getElementType(  ) throw(css::uno::RuntimeException, std::exception) override;
    virtual sal_Bool SAL_CALL SAL_CALL hasElements(  ) throw(css::uno::RuntimeException, std::exception) override;

    //XNameAccess
    virtual css::uno::Any SAL_CALL getByName(const OUString& Name) throw( css::container::NoSuchElementException, css::lang::WrappedTargetException, css::uno::RuntimeException, std::exception ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getElementNames() throw( css::uno::RuntimeException, std::exception ) override;
    virtual sal_Bool SAL_CALL hasByName(const OUString& Name) throw( css::uno::RuntimeException, std::exception ) override;

};

// access to a family of automatic styles (character or paragraph or ...)
class SwXAutoStyleFamily : public cppu::WeakImplHelper< css::style::XAutoStyleFamily >,
    public SwClient
{
    SwDocShell *m_pDocShell;
    IStyleAccess::SwAutoStyleFamily m_eFamily;

protected:
   virtual void Modify( const SfxPoolItem* pOld, const SfxPoolItem *pNew) override;

public:
    SwXAutoStyleFamily(SwDocShell* pDocShell, IStyleAccess::SwAutoStyleFamily eFamily);
    virtual ~SwXAutoStyleFamily();

    //XAutoStyleFamily
    virtual css::uno::Reference< css::style::XAutoStyle > SAL_CALL insertStyle( const css::uno::Sequence< css::beans::PropertyValue >& Values ) throw (css::uno::RuntimeException, std::exception) override;

    //XEnumerationAccess
    virtual css::uno::Reference< css::container::XEnumeration > SAL_CALL createEnumeration(  ) throw (css::uno::RuntimeException, std::exception) override;

    //XElementAccess
    virtual css::uno::Type SAL_CALL SAL_CALL getElementType(  ) throw(css::uno::RuntimeException, std::exception) override;
    virtual sal_Bool SAL_CALL SAL_CALL hasElements(  ) throw(css::uno::RuntimeException, std::exception) override;

};

class SwXAutoStylesEnumerator : public cppu::WeakImplHelper< css::container::XEnumeration >,
    public SwClient
{
    SwAutoStylesEnumImpl *m_pImpl;
public:
    SwXAutoStylesEnumerator( SwDoc* pDoc, IStyleAccess::SwAutoStyleFamily eFam );
    virtual ~SwXAutoStylesEnumerator();

    //XEnumeration
    virtual sal_Bool SAL_CALL hasMoreElements(  ) throw (css::uno::RuntimeException, std::exception) override;
    virtual css::uno::Any SAL_CALL nextElement(  ) throw (css::container::NoSuchElementException, css::lang::WrappedTargetException, css::uno::RuntimeException, std::exception) override;
protected:
    virtual void Modify( const SfxPoolItem* pOld, const SfxPoolItem *pNew) override;
};

// an automatic style
class SwXAutoStyle : public cppu::WeakImplHelper
<
    css::beans::XPropertySet,
    css::beans::XPropertyState,
    css::style::XAutoStyle
>,
    public SwClient
{
private:
    SfxItemSet_Pointer_t                mpSet;
    IStyleAccess::SwAutoStyleFamily     meFamily;
    SwDoc&                              mrDoc;

    css::uno::Sequence< css::uno::Any > SAL_CALL GetPropertyValues_Impl( const css::uno::Sequence< OUString >& aPropertyNames ) throw(css::beans::UnknownPropertyException, css::lang::WrappedTargetException, css::uno::RuntimeException);

public:

    SwXAutoStyle( SwDoc* pDoc, SfxItemSet_Pointer_t pInitSet, IStyleAccess::SwAutoStyleFamily eFam );
    virtual ~SwXAutoStyle();

    //XPropertySet
    virtual css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(  ) throw(css::uno::RuntimeException, std::exception) override;
    virtual void SAL_CALL setPropertyValue( const OUString& aPropertyName, const css::uno::Any& aValue ) throw(css::beans::UnknownPropertyException, css::beans::PropertyVetoException, css::lang::IllegalArgumentException, css::lang::WrappedTargetException, css::uno::RuntimeException, std::exception) override;
    virtual css::uno::Any SAL_CALL getPropertyValue( const OUString& PropertyName ) throw(css::beans::UnknownPropertyException, css::lang::WrappedTargetException, css::uno::RuntimeException, std::exception) override;
    virtual void SAL_CALL addPropertyChangeListener( const OUString& aPropertyName, const css::uno::Reference< css::beans::XPropertyChangeListener >& xListener ) throw(css::beans::UnknownPropertyException, css::lang::WrappedTargetException, css::uno::RuntimeException, std::exception) override;
    virtual void SAL_CALL removePropertyChangeListener( const OUString& aPropertyName, const css::uno::Reference< css::beans::XPropertyChangeListener >& aListener ) throw(css::beans::UnknownPropertyException, css::lang::WrappedTargetException, css::uno::RuntimeException, std::exception) override;
    virtual void SAL_CALL addVetoableChangeListener( const OUString& PropertyName, const css::uno::Reference< css::beans::XVetoableChangeListener >& aListener ) throw(css::beans::UnknownPropertyException, css::lang::WrappedTargetException, css::uno::RuntimeException, std::exception) override;
    virtual void SAL_CALL removeVetoableChangeListener( const OUString& PropertyName, const css::uno::Reference< css::beans::XVetoableChangeListener >& aListener ) throw(css::beans::UnknownPropertyException, css::lang::WrappedTargetException, css::uno::RuntimeException, std::exception) override;

    //XMultiPropertySet
    virtual void SAL_CALL setPropertyValues( const css::uno::Sequence< OUString >& aPropertyNames, const css::uno::Sequence< css::uno::Any >& aValues ) throw (css::beans::PropertyVetoException, css::lang::IllegalArgumentException, css::lang::WrappedTargetException, css::uno::RuntimeException, std::exception) override;
    virtual css::uno::Sequence< css::uno::Any > SAL_CALL getPropertyValues( const css::uno::Sequence< OUString >& aPropertyNames ) throw (css::uno::RuntimeException, std::exception) override;
    virtual void SAL_CALL addPropertiesChangeListener( const css::uno::Sequence< OUString >& aPropertyNames, const css::uno::Reference< css::beans::XPropertiesChangeListener >& xListener ) throw (css::uno::RuntimeException, std::exception) override;
    virtual void SAL_CALL removePropertiesChangeListener( const css::uno::Reference< css::beans::XPropertiesChangeListener >& xListener ) throw (css::uno::RuntimeException, std::exception) override;
    virtual void SAL_CALL firePropertiesChangeEvent( const css::uno::Sequence< OUString >& aPropertyNames, const css::uno::Reference< css::beans::XPropertiesChangeListener >& xListener ) throw (css::uno::RuntimeException, std::exception) override;

    //XPropertyState
    virtual css::beans::PropertyState SAL_CALL getPropertyState( const OUString& PropertyName ) throw(css::beans::UnknownPropertyException, css::uno::RuntimeException, std::exception) override;
    virtual css::uno::Sequence< css::beans::PropertyState > SAL_CALL getPropertyStates( const css::uno::Sequence< OUString >& aPropertyName ) throw(css::beans::UnknownPropertyException, css::uno::RuntimeException, std::exception) override;
    virtual void SAL_CALL setPropertyToDefault( const OUString& PropertyName ) throw(css::beans::UnknownPropertyException, css::uno::RuntimeException, std::exception) override;
    virtual css::uno::Any SAL_CALL getPropertyDefault( const OUString& aPropertyName ) throw(css::beans::UnknownPropertyException, css::lang::WrappedTargetException, css::uno::RuntimeException, std::exception) override;

    //XMultiPropertyStates
    virtual void SAL_CALL setAllPropertiesToDefault(  ) throw (css::uno::RuntimeException, std::exception) override;
    virtual void SAL_CALL setPropertiesToDefault( const css::uno::Sequence< OUString >& aPropertyNames ) throw (css::beans::UnknownPropertyException, css::uno::RuntimeException, std::exception) override;
    virtual css::uno::Sequence< css::uno::Any > SAL_CALL getPropertyDefaults( const css::uno::Sequence< OUString >& aPropertyNames ) throw (css::beans::UnknownPropertyException, css::lang::WrappedTargetException, css::uno::RuntimeException, std::exception) override;

    // Special
    virtual css::uno::Sequence< css::beans::PropertyValue > SAL_CALL getProperties() throw (css::uno::RuntimeException, std::exception) override;
protected:
    virtual void Modify( const SfxPoolItem* pOld, const SfxPoolItem *pNew) override;

};
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
