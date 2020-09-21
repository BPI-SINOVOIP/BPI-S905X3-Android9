package org.bouncycastle.cert;

import java.math.BigInteger;
import java.util.Date;
import java.util.Locale;

import org.bouncycastle.asn1.ASN1Encodable;
import org.bouncycastle.asn1.ASN1Integer;
import org.bouncycastle.asn1.ASN1ObjectIdentifier;
import org.bouncycastle.asn1.x500.X500Name;
import org.bouncycastle.asn1.x509.Certificate;
import org.bouncycastle.asn1.x509.Extension;
import org.bouncycastle.asn1.x509.ExtensionsGenerator;
import org.bouncycastle.asn1.x509.SubjectPublicKeyInfo;
import org.bouncycastle.asn1.x509.Time;
import org.bouncycastle.asn1.x509.V3TBSCertificateGenerator;
import org.bouncycastle.operator.ContentSigner;


/**
 * class to produce an X.509 Version 3 certificate.
 */
public class X509v3CertificateBuilder
{
    private V3TBSCertificateGenerator   tbsGen;
    private ExtensionsGenerator extGenerator;

    /**
     * Create a builder for a version 3 certificate.
     *
     * @param issuer the certificate issuer
     * @param serial the certificate serial number
     * @param notBefore the date before which the certificate is not valid
     * @param notAfter the date after which the certificate is not valid
     * @param subject the certificate subject
     * @param publicKeyInfo the info structure for the public key to be associated with this certificate.
     */
    public X509v3CertificateBuilder(X500Name issuer, BigInteger serial, Date notBefore, Date notAfter, X500Name subject, SubjectPublicKeyInfo publicKeyInfo)
    {
        this(issuer, serial, new Time(notBefore), new Time(notAfter), subject, publicKeyInfo);
    }

    /**
     * Create a builder for a version 3 certificate. You may need to use this constructor if the default locale
     * doesn't use a Gregorian calender so that the Time produced is compatible with other ASN.1 implementations.
     *
     * @param issuer the certificate issuer
     * @param serial the certificate serial number
     * @param notBefore the date before which the certificate is not valid
     * @param notAfter the date after which the certificate is not valid
     * @param dateLocale locale to be used for date interpretation.
     * @param subject the certificate subject
     * @param publicKeyInfo the info structure for the public key to be associated with this certificate.
     */
    public X509v3CertificateBuilder(X500Name issuer, BigInteger serial, Date notBefore, Date notAfter, Locale dateLocale, X500Name subject, SubjectPublicKeyInfo publicKeyInfo)
    {
        this(issuer, serial, new Time(notBefore, dateLocale), new Time(notAfter, dateLocale), subject, publicKeyInfo);
    }

    /**
     * Create a builder for a version 3 certificate.
     *
     * @param issuer the certificate issuer
     * @param serial the certificate serial number
     * @param notBefore the Time before which the certificate is not valid
     * @param notAfter the Time after which the certificate is not valid
     * @param subject the certificate subject
     * @param publicKeyInfo the info structure for the public key to be associated with this certificate.
     */
    public X509v3CertificateBuilder(X500Name issuer, BigInteger serial, Time notBefore, Time notAfter, X500Name subject, SubjectPublicKeyInfo publicKeyInfo)
    {
        tbsGen = new V3TBSCertificateGenerator();
        tbsGen.setSerialNumber(new ASN1Integer(serial));
        tbsGen.setIssuer(issuer);
        tbsGen.setStartDate(notBefore);
        tbsGen.setEndDate(notAfter);
        tbsGen.setSubject(subject);
        tbsGen.setSubjectPublicKeyInfo(publicKeyInfo);

        extGenerator = new ExtensionsGenerator();
    }

    /**
     * Set the subjectUniqueID - note: it is very rare that it is correct to do this.
     *
     * @param uniqueID a boolean array representing the bits making up the subjectUniqueID.
     * @return this builder object.
     */
    public X509v3CertificateBuilder setSubjectUniqueID(boolean[] uniqueID)
    {
        tbsGen.setSubjectUniqueID(CertUtils.booleanToBitString(uniqueID));

        return this;
    }

    /**
     * Set the issuerUniqueID - note: it is very rare that it is correct to do this.
     *
     * @param uniqueID a boolean array representing the bits making up the issuerUniqueID.
     * @return this builder object.
     */
    public X509v3CertificateBuilder setIssuerUniqueID(boolean[] uniqueID)
    {
        tbsGen.setIssuerUniqueID(CertUtils.booleanToBitString(uniqueID));

        return this;
    }

    /**
     * Add a given extension field for the standard extensions tag (tag 3)
     *
     * @param oid the OID defining the extension type.
     * @param isCritical true if the extension is critical, false otherwise.
     * @param value the ASN.1 structure that forms the extension's value.
     * @return this builder object.
     */
    public X509v3CertificateBuilder addExtension(
        ASN1ObjectIdentifier oid,
        boolean isCritical,
        ASN1Encodable value)
        throws CertIOException
    {
        CertUtils.addExtension(extGenerator, oid, isCritical, value);

        return this;
    }

    /**
     * Add a given extension field for the standard extensions tag (tag 3).
     *
     * @param extension the full extension value.
     * @return this builder object.
     */
    public X509v3CertificateBuilder addExtension(
        Extension extension)
        throws CertIOException
    {
        extGenerator.addExtension(extension);

        return this;
    }

    /**
     * Add a given extension field for the standard extensions tag (tag 3) using a byte encoding of the
     * extension value.
     *
     * @param oid the OID defining the extension type.
     * @param isCritical true if the extension is critical, false otherwise.
     * @param encodedValue a byte array representing the encoding of the extension value.
     * @return this builder object.
     */
    public X509v3CertificateBuilder addExtension(
        ASN1ObjectIdentifier oid,
        boolean isCritical,
        byte[] encodedValue)
        throws CertIOException
    {
        extGenerator.addExtension(oid, isCritical, encodedValue);

        return this;
    }

    /**
     * Add a given extension field for the standard extensions tag (tag 3)
     * copying the extension value from another certificate.
     *
     * @param oid the OID defining the extension type.
     * @param isCritical true if the copied extension is to be marked as critical, false otherwise.
     * @param certHolder the holder for the certificate that the extension is to be copied from.
     * @return this builder object.
     */
    public X509v3CertificateBuilder copyAndAddExtension(
        ASN1ObjectIdentifier oid,
        boolean isCritical,
        X509CertificateHolder certHolder)
    {
        Certificate cert = certHolder.toASN1Structure();

        Extension extension = cert.getTBSCertificate().getExtensions().getExtension(oid);

        if (extension == null)
        {
            throw new NullPointerException("extension " + oid + " not present");
        }

        extGenerator.addExtension(oid, isCritical, extension.getExtnValue().getOctets());

        return this;
    }

    /**
     * Generate an X.509 certificate, based on the current issuer and subject
     * using the passed in signer.
     *
     * @param signer the content signer to be used to generate the signature validating the certificate.
     * @return a holder containing the resulting signed certificate.
     */
    public X509CertificateHolder build(
        ContentSigner signer)
    {
        tbsGen.setSignature(signer.getAlgorithmIdentifier());

        if (!extGenerator.isEmpty())
        {
            tbsGen.setExtensions(extGenerator.generate());
        }

        return CertUtils.generateFullCert(signer, tbsGen.generateTBSCertificate());
    }
}